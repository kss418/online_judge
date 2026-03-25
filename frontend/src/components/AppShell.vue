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
      <header class="shell-header">
        <div>
          <p class="shell-header-label">workspace</p>
          <h2>{{ currentSectionTitle }}</h2>
        </div>
        <p class="shell-header-copy">
          초기 뼈대만 잡아 둔 상태라서 다음 작업에서 화면과 폼을 바로 붙일 수 있습니다.
        </p>
      </header>

      <main class="shell-content">
        <RouterView />
      </main>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'

import { apiBaseUrl } from '@/api/http'

const route = useRoute()

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
    hint: '제출 화면 뼈대'
  },
  {
    to: '/login',
    label: 'Login',
    hint: '인증 화면 뼈대'
  }
]

function isActive(path){
  return path === '/'
    ? route.path === '/'
    : route.path.startsWith(path)
}

const currentSectionTitle = computed(() => {
  const activeItem = navItems.find((item) => isActive(item.to))
  return activeItem?.label || 'Frontend Studio'
})
</script>
