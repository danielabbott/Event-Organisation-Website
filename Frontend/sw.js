// Code adapted from https://developers.google.com/web/fundamentals/architecture/app-shell

var cacheName = 'shell-content';
var filesToCache = [
  '/',
  '/style.css',
  '/lib/velocity1.5.2.min.js',
  '/UIVisibility.js',
  '/util.js',
  '/Comments.js',
  '/init.js',
  '/ico/angle-arrow-down.svg',
  '/ico/reorder-option.svg',
  '/ico/bell-musical-tool.svg',
  '/ico/blank-prof-pic.svg',
  '/ico/file.svg',
  '/ico/padlock.svg',
  '/ico/padlock-unlock.svg',
  '/ico/pencil.svg',
  '/ico/trash.svg',
];

self.addEventListener('install', function(e) {
  console.log('[ServiceWorker] Install');
  e.waitUntil(
    caches.open(cacheName).then(function(cache) {
      console.log('[ServiceWorker] Caching app shell');
      return cache.addAll(filesToCache);
    })
  );
});
