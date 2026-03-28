<template>
  <section class="page-grid single-column">
    <article class="panel my-info-panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">account</p>
          <h3>내 정보</h3>
          <p class="my-info-copy">
            현재 로그인한 계정의 기본 정보만 간단히 확인할 수 있습니다.
          </p>
        </div>

        <StatusBadge
          :label="sessionLabel"
          :tone="sessionTone"
        />
      </div>

      <div v-if="authState.isInitializing" class="empty-state">
        <p>내 정보를 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>로그인하면 계정 기본 정보를 여기서 확인할 수 있습니다.</p>
      </div>

      <div v-else class="my-info-summary">
        <div class="metric-row">
          <span class="metric-label">ID</span>
          <strong>#{{ currentUser.id }}</strong>
        </div>
        <div class="metric-row">
          <span class="metric-label">닉네임</span>
          <strong>{{ currentUser.user_name }}</strong>
        </div>
        <div class="metric-row">
          <span class="metric-label">역할</span>
          <div class="my-info-role">
            <strong>{{ currentUser.role_name }}</strong>
            <StatusBadge
              :label="roleLabel"
              :tone="roleTone"
            />
          </div>
        </div>
        <div class="metric-row">
          <span class="metric-label">권한 레벨</span>
          <strong>{{ currentUser.permission_level }}</strong>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted } from 'vue'

import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth } = useAuth()

const currentUser = computed(() => authState.currentUser ?? {
  id: 0,
  user_name: '',
  role_name: 'user',
  permission_level: 0
})

const sessionLabel = computed(() => {
  if (authState.isInitializing) {
    return 'Loading'
  }

  return isAuthenticated.value ? 'Signed In' : 'Guest'
})

const sessionTone = computed(() => {
  if (authState.isInitializing) {
    return 'neutral'
  }

  return isAuthenticated.value ? 'success' : 'neutral'
})

const roleLabel = computed(() => {
  if (currentUser.value.permission_level >= 2) {
    return 'SuperAdmin'
  }

  if (currentUser.value.permission_level >= 1) {
    return 'Admin'
  }

  return 'User'
})

const roleTone = computed(() => {
  if (currentUser.value.permission_level >= 2) {
    return 'danger'
  }

  if (currentUser.value.permission_level >= 1) {
    return 'warning'
  }

  return 'neutral'
})

onMounted(() => {
  initializeAuth()
})
</script>

<style scoped>
.my-info-panel {
  display: grid;
  gap: 1.25rem;
}

.my-info-copy {
  margin: 0.5rem 0 0;
  color: var(--text-muted);
  line-height: 1.6;
}

.my-info-summary {
  display: grid;
  gap: 0.85rem;
}

.my-info-role {
  display: inline-flex;
  align-items: center;
  gap: 0.75rem;
  justify-content: flex-end;
  flex-wrap: wrap;
}
</style>
