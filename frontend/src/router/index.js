import { createRouter, createWebHistory } from 'vue-router'

import HomePage from '@/pages/HomePage.vue'
import ProblemDetailPage from '@/pages/ProblemDetailPage.vue'
import ProblemSubmitPage from '@/pages/ProblemSubmitPage.vue'
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
    path: '/problems/:problemId/submit',
    name: 'problem-submit',
    component: ProblemSubmitPage
  },
  {
    path: '/problems/:problemId',
    name: 'problem-detail',
    component: ProblemDetailPage
  },
  {
    path: '/submissions',
    name: 'submissions',
    component: SubmissionsPage
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
