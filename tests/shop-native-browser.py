#!/usr/bin/env python3
"""Real Chrome click flow against the generated FreeLang app; stdlib only, synthetic data."""
import resource, threading, signal, shutil, base64, hashlib, http.client, json, os, pathlib, secrets, socket, struct, subprocess, tempfile, time, urllib.parse, urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[1]
NATIVE_APP=pathlib.Path(os.environ["FL_SHOP_NATIVE_UI_BIN"]).resolve()
NATIVE_PUBLIC=pathlib.Path(os.environ["FL_SHOP_NATIVE_PUBLIC"]).resolve()

class CDP:
    def __init__(self,url):
        u=urllib.parse.urlsplit(url);self.sock=socket.create_connection((u.hostname,u.port),timeout=10)
        key=base64.b64encode(secrets.token_bytes(16)).decode()
        self.sock.sendall((f'GET {u.path} HTTP/1.1\r\nHost: {u.hostname}:{u.port}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
        response=b''
        while b'\r\n\r\n' not in response:response+=self.sock.recv(1)
        if b' 101 ' not in response.split(b'\r\n')[0]:raise RuntimeError('BROWSER_SOCKET_HANDSHAKE')
        expected=base64.b64encode(hashlib.sha1((key+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest())
        if expected not in response:raise RuntimeError('BROWSER_SOCKET_IDENTITY')
        self.seq=0;self.events=[]
    def exact(self,n):
        out=b''
        while len(out)<n:
            part=self.sock.recv(n-len(out))
            if not part:raise RuntimeError('BROWSER_SOCKET_CLOSED')
            out+=part
        return out
    def send(self,payload,opcode=1):
        body=payload.encode() if isinstance(payload,str) else payload
        n=len(body);head=bytes([0x80|opcode])
        if n<126:head+=bytes([0x80|n])
        elif n<65536:head+=bytes([0x80|126])+struct.pack('!H',n)
        else:head+=bytes([0x80|127])+struct.pack('!Q',n)
        mask=secrets.token_bytes(4)
        self.sock.sendall(head+mask+bytes(b^mask[i%4] for i,b in enumerate(body)))
    def receive(self):
        payload=b''
        while True:
            first,second=self.exact(2);n=second&127
            if n==126:n=struct.unpack('!H',self.exact(2))[0]
            elif n==127:n=struct.unpack('!Q',self.exact(8))[0]
            mask=self.exact(4) if second&128 else None
            body=self.exact(n)
            if mask:body=bytes(b^mask[i%4] for i,b in enumerate(body))
            opcode=first&15
            if opcode==8:raise RuntimeError('BROWSER_SOCKET_CLOSED')
            if opcode==9:self.send(body,10);continue
            if opcode==10:continue
            payload+=body
            if first&128:return json.loads(payload)
    def call(self,method,params=None):
        self.seq+=1;seq=self.seq
        self.send(json.dumps({'id':seq,'method':method,'params':params or {}}))
        while True:
            msg=self.receive()
            if msg.get('id')==seq:
                if 'error' in msg:raise RuntimeError('BROWSER_PROTOCOL_'+method)
                return msg.get('result',{})
            if 'method' in msg:self.events.append(msg)
    def evaluate(self,expression):
        result=self.call('Runtime.evaluate',{'expression':expression,'returnByValue':True,'awaitPromise':True})
        if 'exceptionDetails' in result:raise RuntimeError('BROWSER_EVAL_EXCEPTION')
        return result.get('result',{}).get('value')
    def wait(self,expression,label,seconds=8):
        deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            try:
                if self.evaluate(expression):return
            except RuntimeError:pass
            time.sleep(.05)
        raise AssertionError(label)
    def goto(self,path):
        self.call('Page.navigate',{'url':'http://127.0.0.1:40850'+path})
        self.wait('document.readyState === "complete" && location.pathname === '+json.dumps(path.split('?')[0]),'NAVIGATION')
    def click(self,selector):
        self.wait('!!document.querySelector('+json.dumps(selector)+')','MISSING_CONTROL_'+selector)
        self.evaluate('document.querySelector('+json.dumps(selector)+').click()')
    def fill(self,selector,value):
        self.evaluate('document.querySelector('+json.dumps(selector)+').value='+json.dumps(value))
    def screenshot(self,path,width,height):
        self.call('Emulation.setDeviceMetricsOverride',{'width':width,'height':height,'deviceScaleFactor':1,'mobile':False})
        self.evaluate('document.body.getBoundingClientRect().height')
        if not self.evaluate('document.documentElement.scrollWidth <= innerWidth + 1'):raise AssertionError('RESPONSIVE_HORIZONTAL_OVERFLOW')
        path.write_bytes(base64.b64decode(self.call('Page.captureScreenshot',{'format':'png'})['data']))
    def close(self):self.sock.close()

def run():
    evidence=pathlib.Path(os.environ['FL_NATIVE_UI_EVIDENCE']);evidence.mkdir(parents=True,exist_ok=True)
    with socket.socket() as s:
        try:s.bind(('127.0.0.1',40850))
        except OSError:raise RuntimeError('TEST_PORT_BUSY') from None
    with tempfile.TemporaryDirectory(prefix='fl-shop-browser-') as temporary:
        root=pathlib.Path(temporary);(root/'data').mkdir();(root/'public').mkdir()
        shutil.copytree(NATIVE_PUBLIC,root/'public',dirs_exist_ok=True)
        (root/'public/product-default.svg').write_bytes((ROOT/'web/public/product-default.svg').read_bytes())
        products=[{'id':str(i),'name':f'Synthetic item {i}','price':100*i,'description':'Synthetic fixture','photo':'','stock':5} for i in (1,2)]
        (root/'data/products.json').write_text(json.dumps(products));(root/'data/orders.json').write_text('[]')
        secret=secrets.token_hex(32)
        env={**os.environ,'FL_GIT_REPO':str(root),'FL_ADMIN_PASSWORD':secret,'FL_SHOP_LOCAL_HTTP':'1','FL_HTTP_HOST':'127.0.0.1','FL_HTTP_ALLOW':'127.0.0.1,localhost','FL_HTTP_LOOPBACK_ONLY':'1','FL_SHOP_NATIVE':'1','FL_FILE_BASE':str(root)}
        server_log=open(root/'server.log','wb');chrome_log=open(root/'chrome.log','wb')
        def limit_native():resource.setrlimit(resource.RLIMIT_AS,(2147483648,2147483648))
        server=subprocess.Popen([str(NATIVE_APP)],cwd=root,env=env,stdout=server_log,stderr=subprocess.STDOUT,preexec_fn=limit_native,start_new_session=True)
        chrome=None;pages=[];browser=None
        memory_stop=threading.Event();memory_exceeded=threading.Event();memory_peak=[0];memory_peak_processes=[[]];memory_thread=None
        def process_identity(pid):
            try:
                raw=pathlib.Path('/proc',str(pid),'stat').read_text();fields=raw[raw.rfind(')')+2:].split()
                return int(fields[19])
            except (OSError,ValueError,IndexError):return None
        def memory_watch(owned):
            page_size=os.sysconf('SC_PAGE_SIZE')
            while not memory_stop.wait(.1):
                active=[pid for pid,identity in owned if identity is not None and process_identity(pid)==identity]
                usage=0;members=[]
                for proc in pathlib.Path('/proc').iterdir():
                    if not proc.name.isdigit():continue
                    try:
                        raw=(proc/'stat').read_text();fields=raw[raw.rfind(')')+2:].split()
                        if int(fields[3]) in active:
                            resident=int((proc/'statm').read_text().split()[1])*page_size;usage+=resident
                            members.append({'pid':int(proc.name),'name':(proc/'comm').read_text().strip(),'rss_bytes':resident})
                    except (OSError,ValueError,IndexError):pass
                if usage>memory_peak[0]:memory_peak[0]=usage;memory_peak_processes[0]=members
                if usage>2147483648:
                    memory_exceeded.set()
                    for pid,identity in owned:
                        if identity is not None and process_identity(pid)==identity:
                            try:os.killpg(pid,signal.SIGTERM)
                            except ProcessLookupError:pass
                    return
        def check(label,condition=True):
            if not condition:raise AssertionError(label)
            print(label+'=PASS',flush=True)
        try:
            for _ in range(100):
                if server.poll() is not None:raise RuntimeError('SERVER_START_FAILED')
                try:
                    with urllib.request.urlopen('http://127.0.0.1:40850/login/admin',timeout=1) as response:
                        if response.status==200:break
                except OSError:time.sleep(.05)
            else:raise RuntimeError('SERVER_NOT_READY')
            profile=root/'chrome'
            chrome=subprocess.Popen(['/usr/bin/google-chrome','--headless','--no-sandbox','--disable-gpu','--no-first-run','--disable-background-networking','--disable-extensions','--disable-component-update','--no-zygote','--renderer-process-limit=3','--js-flags=--max-old-space-size=128','--disable-features=BackForwardCache,Prerender2,SpareRendererForSitePerProcess','--remote-debugging-address=127.0.0.1','--remote-debugging-port=0',f'--user-data-dir={profile}','about:blank'],stdout=chrome_log,stderr=subprocess.STDOUT,start_new_session=True)
            memory_thread=threading.Thread(target=memory_watch,args=([(server.pid,process_identity(server.pid)),(chrome.pid,process_identity(chrome.pid))],),daemon=True);memory_thread.start()
            marker=profile/'DevToolsActivePort'
            for _ in range(100):
                if chrome.poll() is not None:raise RuntimeError('CHROME_START_FAILED')
                if marker.exists():break
                time.sleep(.05)
            else:raise RuntimeError('CHROME_NOT_READY')
            port,ws_path=marker.read_text().splitlines()[:2];browser=CDP(f'ws://127.0.0.1:{port}{ws_path}')
            initial_targets=[t['targetId'] for t in browser.call('Target.getTargets')['targetInfos'] if t.get('type')=='page' and t.get('url')=='about:blank']
            def page(context=None):
                context=context or browser.call('Target.createBrowserContext')['browserContextId']
                target=browser.call('Target.createTarget',{'url':'about:blank','browserContextId':context})['targetId']
                with urllib.request.urlopen(f'http://127.0.0.1:{port}/json/list') as response:targets=json.load(response)
                endpoint=next(t['webSocketDebuggerUrl'] for t in targets if t['id']==target)
                tab=CDP(endpoint);tab.context=context;tab.target=target;tab.closed=False;pages.append(tab)
                for domain in ('Page','Runtime','Log','Network'):tab.call(domain+'.enable')
                return tab
            def retire(tab):
                if not tab.closed:
                    check('BROWSER_BEFORE_RETIRE_NO_OVERLAY',tab.evaluate('!document.body.innerText.includes("Island Error")'))
                    browser.call('Target.closeTarget',{'targetId':tab.target})
                    tab.close();tab.closed=True
            a=page();b=page();admin=None
            for target in initial_targets:browser.call('Target.closeTarget',{'targetId':target})
            a.goto('/products')
            a.wait('document.querySelectorAll(".product-card img").length===2 && [...document.querySelectorAll(".product-card img")].every(n=>n.complete && n.naturalWidth>0)','PRODUCT_PHOTOS')
            check('BROWSER_PRODUCT_PHOTOS')
            a.wait('!!document.querySelector(".product-count") && document.querySelector(".product-count").textContent.includes("2개")','NATIVE_PRODUCT_ISLAND')
            check('BROWSER_NATIVE_PRODUCT_ISLAND')
            for width,height in ((375,812),(1280,900)):a.screenshot(evidence/f'products-{width}.png',width,height)
            a.goto('/products/1');a.click('#cart-add')
            a.wait('document.body.innerText.includes("장바구니 개수: 1")','CART_BUTTON_COUNT')
            b.goto('/products/2');b.click('#cart-add');b.wait('document.body.innerText.includes("장바구니 개수: 1")','SECOND_CART_BUTTON')
            a.goto('/cart');a.wait('!!document.querySelector("#cart-items")','CART_MOUNT')
            check('BROWSER_CART_A',a.evaluate('document.querySelector("#cart-items").innerText.includes("Synthetic item 1") && !document.querySelector("#cart-items").innerText.includes("Synthetic item 2")'))
            b.goto('/cart');b.wait('!!document.querySelector("#cart-items")','SECOND_CART_MOUNT')
            check('BROWSER_CART_B',b.evaluate('document.querySelector("#cart-items").innerText.includes("Synthetic item 2") && !document.querySelector("#cart-items").innerText.includes("Synthetic item 1")'))
            for width,height in ((375,812),(1280,900)):b.screenshot(evidence/f'cart-{width}.png',width,height)
            a.goto('/checkout');a.wait('!!document.querySelector("#order-submit") && !document.querySelector("#order-submit").disabled','CHECKOUT_CART_READY')
            a.fill('#phone','010-0000-1234');a.fill('#address','가짜 시험 주소 1');a.fill('#note','합성 메모');a.click('#order-submit')
            a.wait('document.body.innerText.includes("받는 사람은 필수입니다.")','CHECKOUT_VALIDATION')
            check('BROWSER_DRAFT_PRESERVED',a.evaluate('document.querySelector("#phone").value.length===13 && document.querySelector("#address").value.length>5 && document.querySelector("#note").value.length>0'))
            a.fill('#recipient','가짜 손님')
            a.click('#order-submit');a.wait('location.pathname === "/order/done"','ORDER_COMPLETE_NAVIGATION')
            saved=json.loads((root/'data/orders.json').read_text());check('BROWSER_ORDER_SAVED',len(saved)==1)
            oid=saved[0]['order_id'];check('BROWSER_ORDER_RECEIPT',a.evaluate('document.body.innerText.includes('+json.dumps(oid)+')'))
            b.goto('/cart');b.wait('!!document.querySelector("#cart-items")','CART_AFTER_OTHER_ORDER');check('BROWSER_OTHER_CART_PRESERVED',b.evaluate('document.querySelector("#cart-items").innerText.includes("Synthetic item 2")'))
            a.goto('/order/track');a.fill('#track-order-id',oid);a.fill('#track-phone-last4','1234');a.click('#track-submit')
            a.wait('!!document.querySelector("#track-result")','TRACK_RESULT');check('BROWSER_TRACK_OK',a.evaluate('document.querySelector("#track-result").innerText.includes("접수")'))
            check('BROWSER_TRACK_PII_HIDDEN',a.evaluate('!document.querySelector("#track-result").innerText.includes("010-0000-1234") && !document.querySelector("#track-result").innerText.includes("가짜 손님") && !document.querySelector("#track-result").innerText.includes("가짜 시험 주소")'))
            def await_track_status(count,status):
                deadline=time.monotonic()+8
                while time.monotonic()<deadline:
                    a.evaluate('true')
                    replies=[e.get('params',{}) for e in a.events if e.get('method')=='Network.responseReceived' and urllib.parse.urlsplit(e.get('params',{}).get('response',{}).get('url','')).path=='/api/order/track']
                    if len(replies)>=count:
                        reply=replies[count-1]
                        finished=any(e.get('method')=='Network.loadingFinished' and e.get('params',{}).get('requestId')==reply.get('requestId') for e in a.events)
                        if finished:
                            if reply['response']['status']!=status:raise AssertionError('NATIVE_TRACK_RESPONSE_STATUS')
                            a.evaluate('new Promise(resolve=>setTimeout(resolve,0))')
                            return
                    time.sleep(.02)
                raise AssertionError('NATIVE_TRACK_RESPONSE_MISSING')
            # Added Native rendered error/rate-limit coverage; original success assertions remain.
            for attempt in range(10):
                a.fill('#track-order-id',oid);a.fill('#track-phone-last4','9999');a.click('#track-submit')
                await_track_status(attempt+2,404)
                a.wait('!!document.querySelector("#track-error") && document.querySelector("#track-error").textContent.includes("확인할 수 없습니다")','TRACK_WRONG_MESSAGE')
                a.evaluate('true')
                statuses=[e.get('params',{}).get('response',{}).get('status') for e in a.events if e.get('method')=='Network.responseReceived' and urllib.parse.urlsplit(e.get('params',{}).get('response',{}).get('url','')).path=='/api/order/track']
                if len(statuses)<attempt+2 or statuses[-1]!=404:raise AssertionError('NATIVE_TRACK_WRONG_STATUS')
            check('BROWSER_TRACK_WRONG_404')
            a.fill('#track-order-id',oid);a.fill('#track-phone-last4','1234');a.click('#track-submit')
            await_track_status(12,429)
            a.wait('!!document.querySelector("#track-error") && document.querySelector("#track-error").textContent.includes("많습니다")','TRACK_RATE_MESSAGE')
            statuses=[e.get('params',{}).get('response',{}).get('status') for e in a.events if e.get('method')=='Network.responseReceived' and urllib.parse.urlsplit(e.get('params',{}).get('response',{}).get('url','')).path=='/api/order/track']
            check('BROWSER_TRACK_RATE_429',statuses[-1]==429)
            customer_a_context=a.context;customer_b_context=b.context
            retire(a);retire(b)
            admin=page()
            admin.goto('/login/admin');admin.fill('input[name=password]',secret);admin.click('button')
            admin.wait('location.pathname === "/admin/products" && !!document.querySelector("#admin-name")','ADMIN_SESSION_UI')
            admin.wait('document.querySelectorAll(".admin-product").length === 2','ADMIN_AUTOLOAD');check('BROWSER_ADMIN_SESSION_AUTOLOAD')
            for selector,value in [('#admin-name','Synthetic new item'),('#admin-price','150'),('#admin-stock','3'),('#admin-description','Synthetic added fixture')]:admin.fill(selector,value)
            admin.click('#admin-save');admin.wait('document.querySelectorAll(".admin-product").length === 3','ADMIN_ADD')
            check('BROWSER_ADMIN_ADD',len(json.loads((root/'data/products.json').read_text()))==3)
            admin.evaluate('[...document.querySelectorAll(".admin-product")].find(x=>x.innerText.includes("Synthetic new item")).querySelector(".admin-edit").click()')
            admin.fill('#admin-name','Synthetic edited item');admin.fill('#admin-price','175');admin.click('#admin-save')
            admin.wait('document.body.innerText.includes("Synthetic edited item")','ADMIN_EDIT')
            check('BROWSER_ADMIN_EDIT',any(x['name']=='Synthetic edited item' and x['price']==175 for x in json.loads((root/'data/products.json').read_text())))
            admin.goto('/admin/orders');admin.wait('!!document.querySelector(".order-advance")','ORDERS_AUTOLOAD');admin.click('.order-advance')
            admin.wait('[...document.querySelectorAll(".order-row span")].some(n=>n.innerText.includes("원 · 준비중"))','ORDER_ADVANCE');check('BROWSER_ADMIN_STATUS',json.loads((root/'data/orders.json').read_text())[0]['status']=='준비중')
            for width,height in ((375,812),(1280,900)):admin.screenshot(evidence/f'admin-orders-{width}.png',width,height)
            admin.click('#orders-logout');admin.wait('location.pathname === "/login/admin"','ISLAND_LOGOUT')
            check('BROWSER_ISLAND_LOGOUT',admin.evaluate('fetch("/api/admin/orders").then(r=>r.status)')==401)
            retire(admin)
            a=page(customer_a_context);b=page(customer_b_context)
            # Isolated malicious fixture: no real data, credentials, or external destinations.
            attack='<script>globalThis.__native_injection=1</script><img data-native-pwn=x src=x onerror="globalThis.__native_injection=1">'
            fixtures=json.loads((root/'data/products.json').read_text())
            fixtures[1]['name']=attack
            (root/'data/products.json').write_text(json.dumps(fixtures))
            a.goto('/products');a.wait('!!document.querySelector(".product-count")','PRODUCT_INJECTION_ISLAND')
            check('BROWSER_SSR_JSON_ESCAPE',a.evaluate('typeof globalThis.__native_injection === "undefined" && !document.querySelector("img[data-native-pwn]") && JSON.parse(document.querySelector("#product-island-data").textContent)[1].name.startsWith("<script>")'))
            b.goto('/cart');b.wait('!!document.querySelector("#cart-items")','CART_INJECTION_ISLAND')
            check('BROWSER_ISLAND_TEXT_ESCAPE',b.evaluate('typeof globalThis.__native_injection === "undefined" && !document.querySelector("img[data-native-pwn]") && document.querySelector("#cart-items").textContent.includes("<script>")'))
            for tab in pages:
                if not tab.closed:tab.evaluate('true')
            errors=sum(e['method']=='Runtime.exceptionThrown' for tab in pages for e in tab.events)
            check('BROWSER_RUNTIME_EXCEPTIONS_ZERO',errors==0)
            console_errors=[e for tab in pages for e in tab.events if e.get('method')=='Runtime.consoleAPICalled' and e.get('params',{}).get('type')=='error']
            check('BROWSER_CONSOLE_ERRORS_ZERO',not console_errors)
            check('BROWSER_NO_ERROR_OVERLAY',all(tab.evaluate('!document.body.innerText.includes("Island Error") && !document.body.innerText.includes("INSTRUMENTATION_INCOMPLETE")') for tab in pages if not tab.closed))
            check('BROWSER_RSS_WITHIN_2GIB',not memory_exceeded.is_set())
            print('BROWSER_GROUP_PEAK_RSS_BYTES='+str(memory_peak[0]),flush=True)
            print('SHOP_NATIVE_BROWSER=PASS',flush=True)
        except Exception:
            for index,tab in enumerate(pages):
                try:
                    state=tab.evaluate('({path:location.pathname, fields:["recipient","phone","address"].map(id=>{const n=document.getElementById(id);return n?n.value.length:null}),error:document.getElementById("order-submit-error")?.textContent||""})')
                    print('BROWSER_DIAGNOSTIC_'+str(index)+'='+json.dumps(state,ensure_ascii=False),flush=True)
                    for event in tab.events:
                        if event.get('method')=='Runtime.exceptionThrown':
                            details=event.get('params',{}).get('exceptionDetails',{})
                            message=details.get('exception',{}).get('description',details.get('text',''))
                            print('BROWSER_EXCEPTION='+message.split('\n')[0].replace(secret,'<redacted>')[:180],flush=True)
                except Exception:print('BROWSER_DIAGNOSTIC_UNAVAILABLE',flush=True)
            raise
        finally:
            memory_stop.set()
            if memory_thread is not None:memory_thread.join(timeout=2)
            (evidence/'browser-resource-summary.json').write_text(json.dumps({'peak_rss_bytes':memory_peak[0],'budget_exceeded':memory_exceeded.is_set(),'peak_members':memory_peak_processes[0],'native_server_exit_before_cleanup':server.poll(),'chrome_exit_before_cleanup':chrome.poll() if chrome is not None else None},indent=2))
            for name in ['server','chrome']:
                handle=server_log if name=='server' else chrome_log;handle.flush()
                raw=(root/(name+'.log')).read_text(errors='replace').replace(secret,'<redacted>').replace(str(root),'<synthetic-root>')
                (evidence/(name+'-diagnostics.log')).write_text(raw)
            for tab in pages:
                if not tab.closed:tab.close()
            if browser:browser.close()
            if chrome is not None:chrome.terminate();chrome.wait(timeout=10)
            server.terminate();server.wait(timeout=10);server_log.close();chrome_log.close()

if __name__=='__main__':
    try:run()
    except Exception as error:
        import sys,re
        label=str(error)
        print('SHOP_NATIVE_BROWSER=FAIL',flush=True)
        print('FAILURE='+(label if re.fullmatch(r'[A-Za-z0-9_#/-]+',label) else type(error).__name__),flush=True)
        sys.exit(1)
