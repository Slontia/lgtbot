import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', name: 'home', component: () => import('../pages/HomePage.vue') },
    { path: '/login', name: 'login', component: () => import('../pages/LoginPage.vue'), meta: { guestOnly: true } },
    { path: '/lobby', name: 'lobby', component: () => import('../pages/LobbyPage.vue'), meta: { requiresAuth: true } },
    { path: '/game', name: 'game', component: () => import('../pages/GamePage.vue'), meta: { requiresAuth: true } },
    { path: '/history', name: 'history', component: () => import('../pages/HistoryPage.vue'), meta: { requiresAuth: true } },
    { path: '/profile', name: 'profile', component: () => import('../pages/ProfilePage.vue'), meta: { requiresAuth: true } },
  ],
})

router.beforeEach((to) => {
  const auth = useAuthStore()
  if (to.meta.requiresAuth && !auth.token) {
    return { name: 'login', query: { redirect: to.fullPath } }
  }
  if (to.meta.guestOnly && auth.token) {
    return { name: 'lobby' }
  }
  return true
})

export default router
