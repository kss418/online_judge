<template>
  <section class="page-grid single-column">
    <article class="panel admin-users-panel">
      <div class="admin-users-toolbar">
        <div>
          <p class="panel-kicker">Privilege</p>
          <h3>권한 관리</h3>
          <p class="admin-users-copy">
            가입한 사용자를 확인하고 어드민 승격과 유저 강등을 관리할 수 있습니다.
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
        <p>권한 관리 페이지는 로그인한 슈퍼어드민만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageUsers" class="empty-state error-state">
        <p>이 페이지는 슈퍼어드민만 접근할 수 있습니다.</p>
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
              <span class="metric-label">SuperAdmin</span>
              <strong>{{ superAdminCount }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">Admin</span>
              <strong>{{ adminCount }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">User</span>
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
              <div class="admin-user-role">
                <StatusBadge
                  :label="formatPermissionLabel(user.permission_level)"
                  :tone="getPermissionTone(user.permission_level)"
                />
              </div>
              <time
                class="admin-user-created-at"
                :datetime="user.created_at"
              >
                <span
                  class="admin-user-relative-time"
                  tabindex="0"
                >
                  {{ formatRelativeCreatedAt(user.created_at_timestamp) }}
                  <span
                    v-if="user.created_at_label"
                    class="admin-user-time-tooltip"
                  >
                    {{ user.created_at_label }}
                  </span>
                </span>
              </time>
              <div class="admin-user-actions">
                <button
                  v-if="user.permission_level === 0"
                  type="button"
                  class="ghost-button admin-user-action-button"
                  :disabled="!canEditPermissions || savingUserId === user.user_id"
                  @click="handlePromoteToAdmin(user)"
                >
                  {{ savingUserId === user.user_id ? '처리 중...' : '어드민 승격' }}
                </button>
                <button
                  v-else-if="user.permission_level === 1"
                  type="button"
                  class="ghost-button admin-user-action-button"
                  :disabled="!canEditPermissions || savingUserId === user.user_id"
                  @click="handleDemoteToUser(user)"
                >
                  {{ savingUserId === user.user_id ? '처리 중...' : '유저 강등' }}
                </button>
                <span v-else class="admin-user-action-placeholder" aria-hidden="true"></span>
              </div>
            </div>
          </div>
        </div>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'

import { demoteUserToUser, getUserList, promoteUserToAdmin } from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth, refreshCurrentUser } = useAuth()
const isLoading = ref(true)
const errorMessage = ref('')
const actionMessage = ref('')
const savingUserId = ref(0)
const users = ref([])
const nowTimestamp = ref(Date.now())
let latestLoadRequestId = 0
let relativeTimeRefreshTimer = null

const canManageUsers = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 2)
const canEditPermissions = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 2)
const currentUserId = computed(() => Number(authState.currentUser?.id ?? 0))
const superAdminCount = computed(() =>
  users.value.filter((user) => user.permission_level === 2).length
)
const adminCount = computed(() =>
  users.value.filter((user) => user.permission_level === 1).length
)
const regularUserCount = computed(() =>
  users.value.filter((user) => user.permission_level === 0).length
)

watch(
  () => [authState.initialized, authState.token, authState.currentUser?.permission_level],
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

    users.value = responseUsers.map((user) => {
      const permissionLevel = normalizePermissionLevel(user.permission_level)
      return {
        user_id: Number(user.user_id ?? 0),
        user_name: user.user_name ?? '',
        user_login_id: user.user_login_id ?? '',
        permission_level: permissionLevel,
        role_name: user.role_name || getRoleName(permissionLevel),
        created_at: typeof user.created_at === 'string' ? user.created_at : '',
        ...normalizeCreatedAt(user.created_at)
      }
    })
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

async function handlePromoteToAdmin(user){
  return handleRoleAction(user, permissionLevelToRole.admin)
}

async function handleDemoteToUser(user){
  return handleRoleAction(user, permissionLevelToRole.user)
}

async function handleRoleAction(user, nextPermissionLevel){
  if (!authState.token || !canEditPermissions.value) {
    if (!canEditPermissions.value) {
      errorMessage.value = '권한 변경은 슈퍼어드민만 할 수 있습니다.'
    }
    return
  }

  savingUserId.value = user.user_id
  errorMessage.value = ''
  actionMessage.value = ''

  try {
    if (nextPermissionLevel === permissionLevelToRole.admin) {
      await promoteUserToAdmin(user.user_id, authState.token)
    } else {
      await demoteUserToUser(user.user_id, authState.token)
    }

    users.value = users.value.map((currentUser) =>
      currentUser.user_id === user.user_id
        ? {
          ...currentUser,
          permission_level: nextPermissionLevel,
          role_name: getRoleName(nextPermissionLevel)
        }
        : currentUser
    )

    if (user.user_id === currentUserId.value) {
      await refreshCurrentUser()
    }

    actionMessage.value = nextPermissionLevel === permissionLevelToRole.admin
      ? `${user.user_name} 님을 어드민으로 승격했습니다.`
      : `${user.user_name} 님을 유저로 강등했습니다.`
  } catch (error) {
    errorMessage.value = error instanceof Error
      ? error.message
      : '권한 변경을 적용하지 못했습니다.'
  } finally {
    savingUserId.value = 0
  }
}

const permissionLevelToRole = Object.freeze({
  user: 0,
  admin: 1,
  superadmin: 2
})

function normalizePermissionLevel(value){
  const numericValue = Number(value)

  if (Number.isInteger(numericValue) && numericValue >= 0 && numericValue <= 2) {
    return numericValue
  }

  if (Number.isInteger(numericValue) && numericValue >= 100) {
    return 2
  }

  if (Number.isInteger(numericValue) && numericValue >= 10) {
    return 1
  }

  return 0
}

function getRoleName(permissionLevel){
  if (permissionLevel >= 2) {
    return 'superadmin'
  }

  if (permissionLevel >= 1) {
    return 'admin'
  }

  return 'user'
}

function formatPermissionLabel(permissionLevel){
  if (permissionLevel >= 2) {
    return 'SuperAdmin'
  }

  if (permissionLevel >= 1) {
    return 'Admin'
  }

  return 'User'
}

function getPermissionTone(permissionLevel){
  if (permissionLevel >= permissionLevelToRole.superadmin) {
    return 'danger'
  }

  if (permissionLevel >= permissionLevelToRole.admin) {
    return 'warning'
  }

  return 'neutral'
}

function normalizeCreatedAt(value){
  if (typeof value !== 'string' || !value.trim()) {
    return {
      created_at_timestamp: null,
      created_at_label: ''
    }
  }

  const trimmedValue = value.trim()
  const matchedTimestamp = trimmedValue.match(
    /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
  )

  if (matchedTimestamp) {
    const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
      matchedTimestamp
    const normalizedFraction = fractionPart
      ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
      : ''
    const parsedTimestamp = Date.parse(
      `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
    )

    return {
      created_at_timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      created_at_label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
  return {
    created_at_timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    created_at_label: trimmedValue
  }
}

function formatRelativeCreatedAt(timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
  }

  const elapsedSeconds = Math.max(1, Math.floor((nowTimestamp.value - timestamp) / 1000))

  if (elapsedSeconds < 60) {
    return `${elapsedSeconds}초 전`
  }

  const elapsedMinutes = Math.floor(elapsedSeconds / 60)
  if (elapsedMinutes < 60) {
    return `${elapsedMinutes}분 전`
  }

  const elapsedHours = Math.floor(elapsedMinutes / 60)
  if (elapsedHours < 24) {
    return `${elapsedHours}시간 전`
  }

  const elapsedDays = Math.floor(elapsedHours / 24)
  if (elapsedDays < 30) {
    return `${elapsedDays}일 전`
  }

  const elapsedMonths = Math.floor(elapsedDays / 30)
  if (elapsedMonths < 12) {
    return `${elapsedMonths}달 전`
  }

  const elapsedYears = Math.floor(elapsedDays / 365)
  return `${elapsedYears}년 전`
}

function stopRelativeTimeRefresh(){
  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
    relativeTimeRefreshTimer = null
  }
}

function startRelativeTimeRefresh(){
  stopRelativeTimeRefresh()
  nowTimestamp.value = Date.now()
  relativeTimeRefreshTimer = window.setInterval(() => {
    nowTimestamp.value = Date.now()
  }, 1000)
}

onMounted(() => {
  startRelativeTimeRefresh()
  initializeAuth()
})

onUnmounted(() => {
  stopRelativeTimeRefresh()
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
  white-space: pre-line;
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

.admin-users-feedback.is-warning {
  color: var(--warning);
  background: rgba(201, 92, 39, 0.08);
  border-color: rgba(201, 92, 39, 0.18);
}

.admin-users-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(4, minmax(0, 1fr));
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
.admin-user-created-at {
  color: var(--ink-soft);
}

.admin-user-self {
  font-size: 0.86rem;
}

.admin-user-role {
  display: grid;
  gap: 0.3rem;
  justify-items: start;
}

.admin-user-created-at {
  white-space: nowrap;
  font-size: 0.92rem;
  font-weight: 600;
  color: #6C8BCF;
}

.admin-user-relative-time {
  position: relative;
  display: inline-flex;
  align-items: center;
  color: inherit;
  cursor: help;
  outline: none;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.admin-user-relative-time:hover,
.admin-user-relative-time:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.admin-user-time-tooltip {
  position: absolute;
  right: 0;
  bottom: calc(100% + 0.55rem);
  z-index: 10;
  min-width: max-content;
  max-width: 220px;
  padding: 0.55rem 0.7rem;
  border: 1px solid rgba(20, 33, 61, 0.12);
  border-radius: 12px;
  background: rgba(20, 33, 61, 0.96);
  box-shadow: 0 14px 32px rgba(20, 33, 61, 0.18);
  color: #f8fafc;
  font-size: 0.8rem;
  font-weight: 600;
  line-height: 1.4;
  white-space: nowrap;
  pointer-events: none;
  opacity: 0;
  transform: translateY(6px);
  transition:
    opacity 140ms ease,
    transform 140ms ease;
}

.admin-user-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.admin-user-relative-time:hover .admin-user-time-tooltip,
.admin-user-relative-time:focus-visible .admin-user-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.admin-user-actions {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 0.65rem;
}

.admin-user-action-button {
  min-height: 2.4rem;
  padding-inline: 0.95rem;
  white-space: nowrap;
}

.admin-user-action-placeholder {
  display: inline-block;
  min-height: 2.4rem;
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
