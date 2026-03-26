import { createRouter, createWebHistory } from 'vue-router'

import AdminProblemsPage from '@/pages/AdminProblemsPage.vue'
import AdminUsersPage from '@/pages/AdminUsersPage.vue'
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
    path: '/problems/:problemId/submissions',
    name: 'problem-submissions',
    component: SubmissionsPage
  },
  {
    path: '/problems/:problemId/my-submissions',
    name: 'problem-my-submissions',
    component: SubmissionsPage
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
  },
  {
    path: '/admin/problems',
    name: 'admin-problems',
    component: AdminProblemsPage
  },
  {
    path: '/admin/users',
    name: 'admin-users',
    component: AdminUsersPage
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
