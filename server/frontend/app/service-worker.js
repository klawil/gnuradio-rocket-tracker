const cacheName = 'cache-v1';
const precacheResources = [
  '/icons/rocket-pad.png',
  '/icons/rocket-boost.png',
  '/icons/rocket-fast.png',
  '/icons/rocket-coast.png',
  '/icons/rocket-drogue.png',
  '/icons/rocket-main.png',
  '/icons/rocket-landed.png',
];

self.addEventListener('install', e => {
  console.log('service worker installing');
  e.waitUntil(caches.open(cacheName)
    .then(cache => cache.addAll(precacheResources))
    .then(() => self.skipWaiting()));
});

self.addEventListener('activate', e => {
  log.info('service worker activating', e);
});

self.addEventListener('fetch', e => {
  e.respondWith(
    fetch(e.request)
      .then(res => caches.open(cacheName)
        .then(cache => {
          cache.put(e.request, res.clone());
          return res;
      }))
      .catch(() => caches.match(e.request, {
        ignoreSearch: true,
      }).then(res => {
        if (typeof res === 'undefined') {
          return fetch(e.request);
        }
        return res;
      }))
  );
});
