<template>
  <section class="page-grid single-column">
    <article class="panel admin-users-panel">
      <div class="admin-users-toolbar">
        <div>
          <p class="panel-kicker">admin</p>
          <h3>유저 관리</h3>
          <p class="admin-users-copy">
            가입한 사용자를 확인하고 필요한 계정을 관리자로 지정할 수 있습니다.
          </p>
        </div>

        <div class="admin-users-actions">
          <StatusBadge
            :label="isLoading ? 'Loading' : `${users.length} Users`"
            :tone="errorMessage ? 'danger' : 'success'"
          />
          <button
            v-if="canManageUsers"
            type="button"
            class="ghost-button"
            :disabled="isLoading"
            @click="loadUsers"
          >
            새로고침
          </button>
        </div>
      </div>

      <div v-if="authState.isInitializing" class="empty-state">
        <p>관리자 권한을 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>유저 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageUsers" class="empty-state error-state">
        <p>이 페이지는 관리자만 접근할 수 있습니다.</p>
      </div>

      <template v-else>
        <div v-if="actionMessage" class="admin-users-feedback is-success">
          <p>{{ actionMessage }}</p>
        </div>

        <div v-if="isLoading" class="empty-state">
          <p>유저 목록을 불러오는 중입니다.</p>
        </div>

        <div v-else-if="errorMessage" class="empty-state error-state">
          <p>{{ errorMessage }}</p>
        </div>

        <div v-else>
          <div class="admin-users-summary">
            <div class="metric-row">
              <span class="metric-label">전체 사용자</span>
              <strong>{{ users.length }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">관리자</span>
              <strong>{{ adminCount }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">일반 사용자</span>
              <strong>{{ regularUserCount }}</strong>
            </div>
          </div>

          <div v-if="!users.length" class="empty-state">
            <p>표시할 사용자가 아직 없습니다.</p>
          </div>

          <div v-else class="admin-user-table">
            <div class="admin-user-table-head">
              <span>ID</span>
              <span>닉네임</span>
              <span>로그인 ID</span>
              <span>권한</span>
              <span>가입 시각</span>
              <span>관리</span>
            </div>

            <div
              v-for="user in users"
              :key="user.user_id"
              class="admin-user-row"
            >
              <strong>#{{ user.user_id }}</strong>
              <div class="admin-user-name">
                <strong>{{ user.user_name }}</strong>
                <span
                  v-if="user.user_id === currentUserId"
                  class="admin-user-self"
                >
                  현재 계정
                </span>
              </div>
              <span class="admin-user-login-id">{{ user.user_login_id || '-' }}</span>
              <StatusBadge
                :label="user.is_admin ? 'Admin' : 'User'"
                :tone="user.is_admin ? 'warning' : 'neutral'"
              />
              <time
                class="admin-user-created-at"
                :datetime="user.created_at"
              >
                {{ formatCreatedAt(user.created_at) }}
              </time>
              <div class="admin-user-actions">
                <button
                  v-if="!user.is_admin"
                  type="button"
                  class="ghost-button admin-user-action-button"
                  :disabled="promotingUserId === user.user_id"
                  @click="handlePromote(user)"
                >
                  {{ promotingUserId === user.user_id ? '적용 중...' : '관리자 지정' }}
                </button>
                <span v-else class="admin-user-action-label">권한 적용됨</span>
              </div>
            </div>
          </div>
        </div>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'

import { getUserList, promoteUserToAdmin } from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth } = useAuth()
const isLoading = ref(true)
const errorMessage = ref('')
const actionMessage = ref('')
const promotingUserId = ref(0)
const users = ref([])
let latestLoadRequestId = 0

const canManageUsers = computed(() => Boolean(authState.currentUser?.is_admin))
const currentUserId = computed(() => Number(authState.currentUser?.id ?? 0))
const adminCount = computed(() => users.value.filter((user) => user.is_admin).length)
const regularUserCount = computed(() => users.value.length - adminCount.value)

const dateTimeFormatter = new Intl.DateTimeFormat('ko-KR', {
  year: 'numeric',
  month: '2-digit',
  day: '2-digit',
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit',
  hour12: false
})

watch(
  () => [authState.initialized, authState.token, authState.currentUser?.is_admin],
  () => {
    if (!authState.initialized) {
      isLoading.value = true
      return
    }

    if (!isAuthenticated.value || !canManageUsers.value) {
      users.value = []
      errorMessage.value = ''
      actionMessage.value = ''
      isLoading.value = false
      return
    }

    loadUsers()
  },
  { immediate: true }
)

async function loadUsers(){
  if (!authState.token || !canManageUsers.value) {
    return
  }

  const requestId = ++latestLoadRequestId
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getUserList(authState.token)

    if (requestId !== latestLoadRequestId) {
      return
    }

    const responseUsers = Array.isArray(response.users) ? response.users : []
    users.value = responseUsers.map((user) => ({
      user_id: Number(user.user_id ?? 0),
      user_name: user.user_name ?? '',
      user_login_id: user.user_login_id ?? '',
      is_admin: Boolean(user.is_admin),
      created_at: user.created_at ?? ''
    }))
  } catch (error) {
    if (requestId !== latestLoadRequestId) {
      return
    }

    users.value = []
    errorMessage.value = error instanceof Error
      ? error.message
      : '유저 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
  }
}

async function handlePromote(user){
  if (!authState.token || user.is_admin) {
    return
  }

  promotingUserId.value = user.user_id
  errorMessage.value = ''
  actionMessage.value = ''

  try {
    await promoteUserToAdmin(user.user_id, authState.token)
    users.value = users.value.map((currentUser) =>
      currentUser.user_id === user.user_id
        ? { ...currentUser, is_admin: true }
        : currentUser
    )
    actionMessage.value = `${user.user_name} 님을 관리자로 지정했습니다.`
  } catch (error) {
    errorMessage.value = error instanceof Error
      ? error.message
      : '관리자 권한을 적용하지 못했습니다.'
  } finally {
    promotingUserId.value = 0
  }
}

function formatCreatedAt(value){
  if (!value) {
    return '-'
  }

  const normalizedValue = value.includes(' ') ? value.replace(' ', 'T') : value
  const parsedDate = new Date(normalizedValue)

  if (Number.isNaN(parsedDate.getTime())) {
    return value
  }

  return dateTimeFormatter.format(parsedDate)
}

onMounted(() => {
  initializeAuth()
})
</script>

<style scoped>
.admin-users-panel {
  display: grid;
  gap: 1rem;
}

.admin-users-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-users-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
}

.admin-users-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: center;
  justify-content: end;
}

.admin-users-feedback {
  padding: 1rem;
  border-radius: 18px;
  border: 1px solid transparent;
  background: var(--surface-strong);
}

.admin-users-feedback p {
  margin: 0;
}

.admin-users-feedback.is-success {
  color: var(--success);
  background: var(--success-soft);
  border-color: rgba(15, 118, 110, 0.16);
}

.admin-users-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  margin-bottom: 1rem;
}

.admin-user-table {
  border: 1px solid var(--line);
  border-radius: 22px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.68);
}

.admin-user-table-head,
.admin-user-row {
  display: grid;
  grid-template-columns: 0.8fr 1.3fr 1.4fr 0.9fr 1.3fr 1fr;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.15rem;
}

.admin-user-table-head {
  font-size: 0.88rem;
  font-weight: 700;
  color: var(--ink-soft);
  background: rgba(20, 33, 61, 0.04);
  border-bottom: 1px solid var(--line);
}

.admin-user-row + .admin-user-row {
  border-top: 1px solid var(--line);
}

.admin-user-name {
  display: grid;
  gap: 0.2rem;
}

.admin-user-self,
.admin-user-login-id,
.admin-user-created-at,
.admin-user-action-label {
  color: var(--ink-soft);
}

.admin-user-self {
  font-size: 0.86rem;
}

.admin-user-created-at {
  font-size: 0.92rem;
}

.admin-user-actions {
  display: flex;
  justify-content: flex-start;
}

.admin-user-action-button {
  min-height: 2.4rem;
  padding-inline: 0.95rem;
  white-space: nowrap;
}

@media (max-width: 1120px) {
  .admin-users-toolbar {
    flex-direction: column;
  }

  .admin-users-actions {
    justify-content: flex-start;
  }

  .admin-users-summary {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-user-table {
    overflow-x: auto;
  }

  .admin-user-table-head,
  .admin-user-row {
    min-width: 860px;
  }
}
</style>
