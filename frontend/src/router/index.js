import { createRouter, createWebHistory } from 'vue-router'

import { useAuth } from '@/composables/useAuth'
import AdminProblemTestcasesPage from '@/pages/AdminProblemTestcasesPage.vue'
import AdminProblemsPage from '@/pages/AdminProblemsPage.vue'
import AdminUsersPage from '@/pages/AdminUsersPage.vue'
import HomePage from '@/pages/HomePage.vue'
import MyInfoPage from '@/pages/MyInfoPage.vue'
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
    path: '/me',
    name: 'my-info',
    component: MyInfoPage
  },
  {
    path: '/users/:userLoginId',
    name: 'user-info',
    component: MyInfoPage
  },
  {
    path: '/user/:userLoginId',
    redirect: (to) => ({
      name: 'user-info',
      params: {
        userLoginId: String(to.params.userLoginId)
      }
    })
  },
  {
    path: '/admin/problems',
    name: 'admin-problems',
    component: AdminProblemsPage
  },
  {
    path: '/admin/testcases/:problemId?',
    name: 'admin-problem-testcases',
    component: AdminProblemTestcasesPage,
    meta: {
      requiredPermissionLevel: 1
    }
  },
  {
    path: '/admin/problems/:problemId/testcases',
    redirect: (to) => ({
      name: 'admin-problem-testcases',
      params: {
        problemId: String(to.params.problemId)
      }
    })
  },
  {
    path: '/admin/users',
    name: 'admin-users',
    component: AdminUsersPage,
    meta: {
      requiredPermissionLevel: 2
    }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(){
    return { top: 0 }
  }
})

const { authState, initializeAuth } = useAuth()

router.beforeEach(async (to) => {
  const requiredPermissionLevel = Number(to.meta?.requiredPermissionLevel ?? 0)
  if (requiredPermissionLevel <= 0) {
    return true
  }

  if (!authState.initialized) {
    await initializeAuth()
  }

  const currentPermissionLevel = Number(authState.currentUser?.permission_level ?? 0)
  if (currentPermissionLevel >= requiredPermissionLevel) {
    return true
  }

  if (currentPermissionLevel >= 1) {
    return { name: 'admin-problems' }
  }

  return { name: 'home' }
})

export default router
