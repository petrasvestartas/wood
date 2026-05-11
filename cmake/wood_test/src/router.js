import { createRouter, createWebHashHistory } from 'vue-router';
import TestsView from './views/TestsView.vue';

// Use hash mode for GitHub Pages compatibility
// URLs: /wood_test/#/tests
const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    { path: '/', redirect: '/tests' },
    { path: '/tests', component: TestsView }
  ]
});

export default router;
