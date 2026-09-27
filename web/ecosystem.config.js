module.exports = {
  apps: [{
    name: 'web',
    script: './bootstrap-wrapper.js',
    cwd: '/home/kim/kim/projects/fl-git/web',
    wait_ready: true,
    listen_timeout: 8000,
    kill_timeout: 5000,
    autorestart: true,
    env: { PORT: 40870, NODE_ENV: 'production' }
  }]
};
