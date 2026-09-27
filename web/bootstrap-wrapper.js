const http = require('http');
const servers = [];
const orig = http.Server.prototype.listen;
http.Server.prototype.listen = function (...args) {
  servers.push(this);
  this.on('listening', () => { if (process.send) process.send('ready'); });
  return orig.apply(this, args);
};
process.on('SIGINT', () => {
  Promise.all(servers.map(s => new Promise(r => s.close(r)))).then(() => process.exit(0));
  setTimeout(() => process.exit(0), 4500);
});
process.argv = ['node', "/home/kim/kim/projects/fl-git/web/runtime/bootstrap.js", 'run', "/home/kim/kim/projects/fl-git/web/_app.fl"];
require("/home/kim/kim/projects/fl-git/web/runtime/bootstrap.js");
