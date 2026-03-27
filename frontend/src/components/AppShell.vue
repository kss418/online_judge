<template>
  <div class="app-shell">
    <aside class="shell-sidebar">
      <div class="brand-block">
        <h1>Online Judge</h1>
        <p class="brand-copy">
          문제를 탐색하고 코드를 제출한 뒤 결과와 기록을 확인할 수 있습니다.
        </p>
      </div>

      <nav class="nav-list" aria-label="Primary">
        <RouterLink
          v-for="item in navItems"
          :key="item.to"
          :to="item.to"
          class="nav-link"
          :class="{ 'is-active': isActive(item.to) }"
        >
          <span class="nav-label">{{ item.label }}</span>
          <span class="nav-hint">{{ item.hint }}</span>
        </RouterLink>
      </nav>
    </aside>

    <div class="shell-main">
      <div class="shell-topbar">
        <div class="auth-toolbar">
          <template v-if="authState.isInitializing">
            <div class="auth-session-card">
              <p class="auth-session-label">session</p>
              <strong>세션 확인 중...</strong>
            </div>
          </template>

          <template v-else-if="isAuthenticated">
            <div class="auth-session-card">
              <p class="auth-session-label">signed in</p>
              <div class="auth-session-user">
                <strong>{{ authState.currentUser.user_name }}</strong>
                <StatusBadge
                  v-if="authState.currentUser.permission_level >= 1"
                  :label="getRoleBadgeLabel(authState.currentUser)"
                  :tone="getRoleBadgeTone(authState.currentUser)"
                />
              </div>
            </div>

            <button
              type="button"
              class="ghost-button"
              :disabled="authState.isSubmitting"
              @click="handleLogout"
            >
              {{ authState.isSubmitting ? '처리 중...' : '로그아웃' }}
            </button>
          </template>

          <template v-else>
            <button
              type="button"
              class="ghost-button"
              @click="openAuthDialog('sign-up')"
            >
              회원가입
            </button>
            <button
              type="button"
              class="primary-button"
              @click="openAuthDialog('login')"
            >
              로그인
            </button>
          </template>
        </div>
      </div>

      <GlobalNotice />

      <main class="shell-content">
        <RouterView />
      </main>
    </div>
  </div>

  <AuthDialog
    :open="isAuthDialogOpen"
    :initial-mode="authDialogMode"
    @close="closeAuthDialog"
  />
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'

import AuthDialog from '@/components/AuthDialog.vue'
import GlobalNotice from '@/components/GlobalNotice.vue'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const route = useRoute()
const isAuthDialogOpen = ref(false)
const authDialogMode = ref('login')
const { authState, isAuthenticated, initializeAuth, logout } = useAuth()

const navItems = computed(() => {
  const items = [
    {
      to: '/',
      label: 'Overview',
      hint: '서비스 안내와 상태'
    },
    {
      to: '/problems',
      label: 'Problems',
      hint: '문제 목록'
    },
    {
      to: '/submissions',
      label: 'Submissions',
      hint: '제출 목록'
    }
  ]

  if (authState.currentUser?.permission_level >= 1) {
    items.push({
      to: '/admin/problems',
      label: 'Problem Admin',
      hint: '문제 관리'
    })

    items.push({
      to: '/admin/testcases',
      label: 'Testcase Admin',
      hint: '테스트케이스 관리'
    })

    if (authState.currentUser.permission_level >= 2) {
      items.push({
        to: '/admin/users',
        label: 'Privilege',
        hint: '권한 관리'
      })
    }
  }

  return items
})

function isActive(path){
  return path === '/'
    ? route.path === '/'
    : route.path.startsWith(path)
}

function openAuthDialog(mode){
  authDialogMode.value = mode
  isAuthDialogOpen.value = true
}

function closeAuthDialog(){
  if (authState.isSubmitting) {
    return
  }

  isAuthDialogOpen.value = false
}

async function handleLogout(){
  await logout()
}

function getRoleBadgeLabel(user){
  if (!user) {
    return 'User'
  }

  return user.role_name === 'superadmin' ? 'SuperAdmin' : 'Admin'
}

function getRoleBadgeTone(user){
  if (!user) {
    return 'neutral'
  }

  return user.role_name === 'superadmin' ? 'danger' : 'warning'
}

onMounted(() => {
  initializeAuth()
})
</script>
