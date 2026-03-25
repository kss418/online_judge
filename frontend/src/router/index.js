import { createRouter, createWebHistory } from 'vue-router'

import HomePage from '@/pages/HomePage.vue'
import LoginPage from '@/pages/LoginPage.vue'
import ProblemsPage from '@/pages/ProblemsPage.vue'
import SubmissionsPage from '@/pages/SubmissionsPage.vue'

const routes = [
  {
    path: '/',
    name: 'home',
    component: HomePage
  },
  {
    path: '/problems',
    name: 'problems',
    component: ProblemsPage
  },
  {
    path: '/submissions',
    name: 'submissions',
    component: SubmissionsPage
  },
  {
    path: '/login',
    name: 'login',
    component: LoginPage
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(){
    return { top: 0 }
  }
})

export default router
