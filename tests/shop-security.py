#!/usr/bin/env python3
"""Synthetic HTTP verification of the real FreeLang commerce handlers (two processes)."""
import concurrent.futures, hashlib, http.client, json, os, pathlib, secrets, socket, subprocess, tempfile, time
ROOT = pathlib.Path(__file__).resolve().parents[1]
PORTS = (40850, 40851)

def request(port, method, path, body=None, cookie='', headers=None, client_ip='127.0.0.1'):
    conn = http.client.HTTPConnection('127.0.0.1', port, timeout=20, source_address=(client_ip,0))
    hdr = dict(headers or {})
    if cookie: hdr['Cookie'] = cookie
    payload = None
    if body is not None:
        hdr['Content-Type'] = 'application/json'
        payload = json.dumps(body, ensure_ascii=False).encode("utf-8")
    try:
        conn.request(method, path, payload, hdr)
        res = conn.getresponse(); raw = res.read()
        try: data = json.loads(raw)
        except ValueError: data = raw.decode()
        return res.status, data, dict(res.getheaders())
    finally: conn.close()

def run():
    if not (ROOT/'web/_app.fl').is_file(): raise RuntimeError('GENERATED_APP_REQUIRED')
    for port in PORTS:
        with socket.socket() as probe:
            try: probe.bind(('127.0.0.1', port))
            except OSError: raise RuntimeError('TEST_PORT_BUSY') from None
    with tempfile.TemporaryDirectory(prefix='fl-shop-security-') as temporary:
        root = pathlib.Path(temporary); (root/'data').mkdir()
        product = lambda id, stock: {'id':id,'name':'Synthetic '+id,'price':10,'stock':stock,'photo':'','description':''}
        (root/'data/products.json').write_text(json.dumps([product('1',20),product('2',20),product('limited',1)]))
        (root/'data/orders.json').write_text('[]')
        sources = ['_00-shop.flx','_products-store.flx','_orders-store.flx','_cart-api.flx','_admin-api.flx','_layout.flx','_order-api.flx']
        source = '\n'.join((ROOT/'web/pages'/f).read_text() for f in sources)
        for slug in ('products','orders'):
            source += '\n' + (ROOT/f'web/pages/admin/{slug}.flx').read_text().replace('(defn render [req]',f'(defn shop-test-{slug} [req]')
            source += f'\n(server-get "/admin/{slug}" "shop-test-{slug}")\n'
        source += '\n(defn shop-test-ready [req] (server_json {"ready" true}))\n(server-get "/__ready" "shop-test-ready")\n(server_start (str-to-num (env-get "PORT")))\n'
        entry=root/'server.fl';entry.write_text(source)
        secret=secrets.token_hex(32)
        env={**os.environ,'FL_GIT_REPO':str(root),'FL_ADMIN_PASSWORD':secret,'FL_SHOP_LOCAL_HTTP':'1','FL_HTTP_HOST':'127.0.0.1'}
        processes={};logs={}
        def start(port):
            logs[port]=open(root/f'server-{port}.log','ab')
            proc=subprocess.Popen(['node',str(ROOT/'web/runtime/bootstrap.js'),'run',str(entry)],cwd=root,env={**env,'PORT':str(port)},stdout=logs[port],stderr=subprocess.STDOUT)
            processes[port]=proc
            for _ in range(100):
                if proc.poll() is not None:
                    # Do not display request payloads, passwords, or paths from interpreter logs.
                    raise RuntimeError('SYNTHETIC_SERVER_START_FAILED')
                try:
                    if request(port,'GET','/__ready')[0]==200: return
                except (OSError,TimeoutError): pass
                time.sleep(.05)
            raise RuntimeError('SYNTHETIC_SERVER_NOT_READY')
        def stop(port):
            proc=processes.pop(port);proc.terminate();proc.wait(timeout=10);logs.pop(port).close()
        def check(label,condition):
            if not condition: raise AssertionError(label)
            print(label+'=PASS',flush=True)
        try:
            start(PORTS[0]); start(PORTS[1]); p=PORTS[0]
            a=request(p,'GET','/api/cart'); b=request(p,'GET','/api/cart')
            ca=a[2].get('Set-Cookie','').split(';')[0];cb=b[2].get('Set-Cookie','').split(';')[0]
            check('SESSION_DISTINCT', ca.startswith('fl_cart=') and cb.startswith('fl_cart=') and len(ca)==72 and ca!=cb)
            check('COOKIE_FLAGS','HttpOnly' in a[2]['Set-Cookie'] and 'SameSite=Strict' in a[2]['Set-Cookie'])
            aa=request(p,'POST','/api/cart/add',{'id':'1','quantity':2},ca)
            bb=request(p,'POST','/api/cart/add',{'id':'2','quantity':3},cb)
            check('CART_ISOLATION',aa[0]==200 and bb[0]==200 and [i['id'] for i in request(p,'GET','/api/cart',cookie=ca)[1]['items']]==['1'] and [i['id'] for i in request(p,'GET','/api/cart',cookie=cb)[1]['items']]==['2'])
            check('CART_CROSS_PROCESS',request(PORTS[1],'GET','/api/cart',cookie=ca)[1]['count']==2 and request(PORTS[1],'GET','/api/cart',cookie=cb)[1]['count']==3)
            order=lambda id: {'request_id':secrets.token_hex(16),'items':[{'id':id,'quantity':1,'price':1}],'recipient':'가짜 손님','phone':'010-0000-1234','address':'가짜 시험 주소 1'}
            first_body=order('1')
            created=request(p,'POST','/api/order',first_body,ca)
            check('ORDER_SAVED',created[0]==200)
            oid=created[1]['order_id']
            check('CLEAR_ONLY_BUYER',request(p,'GET','/api/cart',cookie=ca)[1]['count']==0 and request(p,'GET','/api/cart',cookie=cb)[1]['count']==3)
            check('SERVER_PRICE',created[1]['total']==10)
            before_retry={name:(root/'data'/name).read_bytes() for name in ('products.json','orders.json')}
            retry=request(p,'POST','/api/order',first_body,ca)
            check('ORDER_RETRY_SAME_RECEIPT',retry[0]==200 and retry[1]==created[1] and all((root/'data'/name).read_bytes()==raw for name,raw in before_retry.items()))
            conflicting={**first_body,'items':[{'id':'1','quantity':2}]}
            check('ORDER_RETRY_CONFLICT_409',request(p,'POST','/api/order',conflicting,ca)[0]==409)
            check('ORDER_INVALID_ITEMS_400',request(p,'POST','/api/order',{**first_body,'items':{}},ca)[0]==400)
            check('CART_INVALID_QUANTITY_400',request(p,'POST','/api/cart/add',{'id':'1','quantity':1e20},ca)[0]==400)

            import threading
            barrier=threading.Barrier(10)
            def buy(index):
                barrier.wait()
                return request(PORTS[index%2],'POST','/api/order',order('limited'))
            with concurrent.futures.ThreadPoolExecutor(max_workers=10) as pool:
                results=list(pool.map(buy,range(10)))
            stored=json.loads((root/'data/orders.json').read_text()); products=json.loads((root/'data/products.json').read_text())
            success=sum(x[0]==200 for x in results)
            check('CONCURRENT_TEN_STOCK_ONE',success==1 and sum(any(i['id']=='limited' for i in x['items']) for x in stored)==1 and next(x['stock'] for x in products if x['id']=='limited')==0)
            print('CONCURRENT_STATUS_COUNTS='+json.dumps({str(s):sum(x[0]==s for x in results) for s in sorted(set(x[0] for x in results))}),flush=True)
            check('OVER_STOCK_400',request(p,'POST','/api/order',order('limited'))[0]==400)
            check('BAD_INPUT_400',request(p,'POST','/api/order',{'items':[]})[0]==400)
            ids=[x['order_id'] for x in stored]
            check('RANDOM_ORDER_ID',all(x.startswith('ORD-') and len(x)==52 for x in ids) and len(set(ids))==len(ids))
            guessed='ORD-'+str(int(time.time()*1000))+'-0'
            check('SEQUENTIAL_ID_GUESS_404',request(p,'POST','/api/order/track',{'order_id':guessed,'phone_last4':'1234'})[0]==404)
            track={'order_id':oid,'phone_last4':'1234'}
            reply=request(p,'POST','/api/order/track',track)
            check('TRACK_PII_HIDDEN',reply[0]==200 and set(reply[1])=={'status','created_at','items','total'})
            (root/'data/security-limits.json').write_text('{}')
            for _ in range(10):
                check('TRACK_WRONG_404',request(p,'POST','/api/order/track',{'order_id':oid,'phone_last4':'9999'})[0]==404)
            check('TRACK_LIMIT_429',request(p,'POST','/api/order/track',track)[0]==429)
            check('TRACK_ORDER_LIMIT_OTHER_IP',request(p,'POST','/api/order/track',track,client_ip='127.0.0.2')[0]==429)
            check('TRACK_OTHER_ORDER_OTHER_IP',request(p,'POST','/api/order/track',{'order_id':'nonexistent-synthetic','phone_last4':'1234'},client_ip='127.0.0.2')[0]==404)
            stop(p); start(p)
            check('TRACK_RESTART_ORDER_LIMIT_OTHER_IP',request(p,'POST','/api/order/track',track,client_ip='127.0.0.3')[0]==429)
            check('ORDER_RETRY_AFTER_RESTART',request(p,'POST','/api/order',first_body,ca)[1]==created[1])
            check('TRACK_RESTART_LIMIT_429',request(p,'POST','/api/order/track',track)[0]==429)
            check('ADMIN_PAGE_401',request(p,'GET','/admin/products')[0]==401 and request(p,'GET','/admin/orders')[0]==401)
            # Start a fresh 15-minute admin window; only synthetic state is changed by this fixture setup.
            (root/'data/security-limits.json').write_text('{}')
            for _ in range(5):
                check('ADMIN_WRONG_401',request(p,'POST','/api/admin/products',{},headers={'X-Admin-Password':'wrong-synthetic'})[0]==401)
            check('ADMIN_SIXTH_429',request(p,'POST','/api/admin/products',{},headers={'X-Admin-Password':secret})[0]==429)
            stop(p); start(p)
            check('ADMIN_RESTART_LOCK_429',request(p,'POST','/api/admin/products',{},headers={'X-Admin-Password':secret})[0]==429)
            (root/'data/security-limits.json').write_text('{}')
            login=request(p,'POST','/api/admin/login',{'password':secret})
            admin_cookie=login[2].get('Set-Cookie','').split(';')[0]
            check('ADMIN_LOGIN',login[0] in (302,303) and admin_cookie.startswith('fl_admin='))
            check('ADMIN_PAGE_AUTH',request(p,'GET','/admin/products',cookie=admin_cookie)[0]==200 and request(p,'GET','/admin/orders',cookie=admin_cookie)[0]==200)
            check('ADMIN_API_AUTH',request(p,'GET','/api/admin/orders',cookie=admin_cookie)[0]==200)
            check('ADMIN_COOKIE_CANNOT_REAUTH_BAD_PASSWORD',request(p,'POST','/api/admin/login',{'password':'wrong-synthetic'},cookie=admin_cookie)[0]==401)
            check('ADMIN_UNSAFE_PRICE_400',request(p,'POST','/api/admin/product',{'name':'Synthetic unsafe','price':1e20,'stock':1},cookie=admin_cookie)[0]==400)
            check('ADMIN_UNSAFE_STOCK_400',request(p,'POST','/api/admin/product',{'name':'Synthetic unsafe','price':10,'stock':1e20},cookie=admin_cookie)[0]==400)
            for status in ('준비중','발송','완료'):
                check('ADMIN_VALID_STATUS',request(p,'PUT','/api/admin/order/'+oid,{'status':status},cookie=admin_cookie)[0]==200)
            done_bytes=(root/'data/orders.json').read_bytes()
            check('ADMIN_TERMINAL_STATUS_400',request(p,'PUT','/api/admin/order/'+oid,{},cookie=admin_cookie)[0]==400 and (root/'data/orders.json').read_bytes()==done_bytes)
            # These are isolated malformed input fixtures, not corruption of real data or crash injection.
            originals={name:(root/'data'/name).read_bytes() for name in ('products.json','orders.json','security-limits.json')}
            for filename,invalid in [('orders.json',{}),('orders.json',[{}]),('products.json',{}),('products.json',[{}])]:
                fixture=json.dumps(invalid).encode();(root/'data'/filename).write_bytes(fixture)
                check('STORAGE_SHAPE_FAIL_CLOSED',request(p,'POST','/api/order',order('1'),ca)[0]==503 and (root/'data'/filename).read_bytes()==fixture)
                for name,raw in originals.items():(root/'data'/name).write_bytes(raw)
            bad_limits={'admin-ip:'+hashlib.sha256(b'127.0.0.1').hexdigest():{'count':'broken','until':int(time.time()*1000)+900000}}
            fixture=json.dumps(bad_limits).encode();(root/'data/security-limits.json').write_bytes(fixture)
            check('LIMIT_STORAGE_FAIL_CLOSED',request(p,'GET','/api/admin/orders',cookie=admin_cookie)[0]==503 and (root/'data/security-limits.json').read_bytes()==fixture)
            (root/'data/security-limits.json').write_bytes(originals['security-limits.json'])

            logout=request(p,'POST','/api/admin/logout',{},cookie=admin_cookie)
            check('ADMIN_LOGOUT_REVOKES',logout[0] in (302,303) and 'Max-Age=0' in logout[2].get('Set-Cookie','') and request(p,'GET','/admin/products',cookie=admin_cookie)[0]==401)
            check('LOCAL_FILES_PRIVATE',all((root/'data'/name).stat().st_mode & 0o077 == 0 for name in ('products.json','orders.json','security-limits.json')))
            check('JOURNAL_CLEAN',not (root/'data/commerce-journal.json').exists())
            for port in list(processes): stop(port)
            (root/'data/security-limits.json').write_text('{}')
            del env['FL_ADMIN_PASSWORD']
            start(p)
            check('ADMIN_UNCONFIGURED_401',request(p,'POST','/api/admin/products',{},headers={'X-Admin-Password':secret})[0]==401)
            stop(p)
            env['FL_ADMIN_PASSWORD']=secret
            generated = ROOT/'web/_app.fl'
            if generated.exists():
                entry.write_text(generated.read_text()+\
                    '\n(defn shop-test-ready [req] (server_json {"ready" true}))\n(server-get "/__ready" "shop-test-ready")\n')
                start(p)
                check('FULL_APP_LOGIN_PAGE',request(p,'GET','/login/admin')[0]==200)
                (root/'data/security-limits.json').write_text('{}')
                check('FULL_APP_ADMIN_401',request(p,'GET','/admin/products')[0]==401)
                # Verify the actual HTML form encoding, without storing or displaying its secret.
                import urllib.parse
                form=urllib.parse.urlencode({'password':secret}).encode()
                connection=http.client.HTTPConnection('127.0.0.1',p,timeout=20)
                connection.request('POST','/api/admin/login',form,{'Content-Type':'application/x-www-form-urlencoded'})
                response=connection.getresponse();response.read()
                cookie=response.getheader('Set-Cookie','').split(';')[0]
                check('FULL_APP_FORM_LOGIN',response.status in (302,303) and cookie.startswith('fl_admin='));connection.close()
                check('FULL_APP_ADMIN_AUTH',request(p,'GET','/admin/products',cookie=cookie)[0]==200)
                check('FULL_APP_CART',request(p,'GET','/cart',cookie=cb)[0]==200 and request(p,'GET','/api/cart',cookie=cb)[1]['count']==3)
                if os.environ.get('SHOP_BROWSER_CHECK')=='1':
                    evidence=ROOT/'.tmp/shop-review'
                    for width,height in ((375,812),(1280,900)):
                        target=evidence/f'login-{width}.png'
                        browser=subprocess.run(['/usr/bin/google-chrome','--headless','--no-sandbox','--disable-gpu','--no-first-run','--disable-background-networking',f'--user-data-dir={root}/chrome-{width}',f'--window-size={width},{height}',f'--screenshot={target}','--dump-dom',f'http://127.0.0.1:{p}/login/admin'],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
                        dom=browser.stdout.decode(errors='replace')
                        check('LOGIN_RENDER_'+str(width),browser.returncode==0 and '<title>관리자 로그인</title>' in dom and 'type="password"' in dom and target.exists() and target.read_bytes().startswith(b'\x89PNG'))
                print('FULL_APP=PASS',flush=True)
            else: raise RuntimeError('GENERATED_APP_REQUIRED')
            print('SHOP_SECURITY=PASS',flush=True)
        except Exception:
            # Preserve only program diagnostics, not access logs or runtime data.
            for port,handle in logs.items(): handle.flush()
            for port in logs:
                lines=(root/f'server-{port}.log').read_text(errors='replace').splitlines()
                for line in lines:
                    if 'Function not found:' in line or 'SHOP_' in line or '[E_' in line:
                        print('DIAGNOSTIC='+line[:250].replace(str(root),'<가림>'))
            raise
        finally:
            for port in list(processes): stop(port)

if __name__=='__main__':
    try:run()
    except Exception as error:
        import sys,re
        label=str(error)
        print('SHOP_SECURITY=FAIL',flush=True)
        print('FAILURE='+(label if re.fullmatch(r'[A-Za-z0-9_]+',label) else type(error).__name__),flush=True)
        sys.exit(1)
