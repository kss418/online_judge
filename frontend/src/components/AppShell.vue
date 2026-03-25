<template>
  <div class="app-shell">
    <aside class="shell-sidebar">
      <div class="brand-block">
        <p class="brand-eyebrow">online judge</p>
        <h1>Frontend Studio</h1>
        <p class="brand-copy">
          백엔드 API와 연결되는 Vue + Vite 작업 공간입니다.
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

      <div class="sidebar-panel">
        <p class="sidebar-panel-label">API Base</p>
        <code>{{ apiBaseUrl }}</code>
      </div>
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
                  v-if="authState.currentUser.is_admin"
                  label="Admin"
                  tone="warning"
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
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'

import { apiBaseUrl } from '@/api/http'
import AuthDialog from '@/components/AuthDialog.vue'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const route = useRoute()
const isAuthDialogOpen = ref(false)
const authDialogMode = ref('login')
const { authState, isAuthenticated, initializeAuth, logout } = useAuth()

const navItems = [
  {
    to: '/',
    label: 'Overview',
    hint: 'backend 연결 상태'
  },
  {
    to: '/problems',
    label: 'Problems',
    hint: '문제 화면 뼈대'
  },
  {
    to: '/submissions',
    label: 'Submissions',
    hint: '제출 목록'
  }
]

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

onMounted(() => {
  initializeAuth()
})
</script>
