const { createProxyMiddleware } = require('http-proxy-middleware');

module.exports = function(app) {
  const target = 'http://astropowerbox-xl-1.lan'
  console.log(`****** proxy setup: "/api" => ${target}`)
  app.use(
    '/api',
    createProxyMiddleware({
      target,
      changeOrigin: true,
      headers: {
        Connection: 'keep-alive'
      }
    })
  );
};
