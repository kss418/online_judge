<template>
  <section class="page-grid single-column">
    <article class="panel admin-user-management-panel">
      <div class="admin-user-management-toolbar">
        <div>
          <p class="panel-kicker">operations</p>
          <h3>유저 관리</h3>
          <p class="admin-user-management-copy">
            가입한 사용자를 확인하고 제출 금지 상태를 운영 관점에서 관리할 수 있습니다.
          </p>
        </div>

        <div class="admin-user-management-actions">
          <StatusBadge
            :label="isLoading ? 'Loading' : `${filteredUsers.length} Users`"
            :tone="errorMessage ? 'danger' : 'success'"
          />

          <div class="admin-user-search-row">
            <form class="admin-user-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="admin-user-management-search">유저 검색</label>
              <input
                id="admin-user-management-search"
                v-model.trim="searchInput"
                class="admin-user-search-input"
                type="search"
                placeholder="ID 검색"
              />
              <button
                type="submit"
                class="primary-button admin-user-search-button"
                :disabled="isLoading"
              >
                검색
              </button>
            </form>

            <button
              v-if="appliedQuery"
              type="button"
              class="ghost-button admin-user-reset-button"
              :disabled="isLoading"
              @click="resetSearch"
            >
              검색 초기화
            </button>
          </div>

          <button
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
        <div class="admin-user-management-summary">
          <div class="metric-row">
            <span class="metric-label">전체 사용자</span>
            <strong>{{ users.length }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">정상</span>
            <strong>{{ normalUserCount }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">제출 금지</span>
            <strong>{{ activeBanCount }}</strong>
          </div>
        </div>

        <div v-if="isLoading" class="empty-state">
          <p>유저 목록을 불러오는 중입니다.</p>
        </div>

        <div v-else-if="errorMessage" class="empty-state error-state">
          <p>{{ errorMessage }}</p>
        </div>

        <div v-else-if="!filteredUsers.length" class="empty-state">
          <p>{{ appliedQuery ? '검색 결과가 없습니다.' : '표시할 사용자가 아직 없습니다.' }}</p>
        </div>

        <div v-else class="admin-user-management-table">
          <div class="admin-user-management-head">
            <span>계정 번호</span>
            <span>ID</span>
            <span>제출 밴</span>
            <span>관리</span>
          </div>

          <div
            v-for="user in pagedUsers"
            :key="user.user_id"
            class="admin-user-management-row"
          >
            <strong>#{{ countFormatter.format(user.user_id) }}</strong>

            <div class="admin-user-identity">
              <RouterLink
                class="admin-user-link"
                :to="{ name: 'user-info', params: { userLoginId: user.user_login_id } }"
              >
                <strong>{{ user.user_login_id || '-' }}</strong>
              </RouterLink>
            </div>

            <div class="admin-user-ban-status">
              <StatusBadge
                :label="getSubmissionBanLabel(user)"
                :tone="getSubmissionBanTone(user)"
              />
              <strong
                v-if="formatSubmissionBanWindow(user)"
                class="admin-user-ban-copy"
              >
                {{ formatSubmissionBanWindow(user) }}
              </strong>
              <span
                v-if="shouldShowSubmissionBanUntil(user)"
                class="admin-user-ban-until"
              >
                해제 시각 {{ user.submission_banned_until_label }}
              </span>
            </div>

            <div class="admin-user-ban-actions">
              <div class="admin-user-ban-presets">
                <button
                  v-for="preset in durationPresets"
                  :key="`${user.user_id}-${preset.minutes}`"
                  type="button"
                  class="ghost-button admin-user-ban-preset"
                  :disabled="isBusyUser(user.user_id)"
                  @click="setDurationDraft(user.user_id, preset.minutes)"
                >
                  {{ preset.label }}
                </button>
              </div>

              <div class="admin-user-ban-form">
                <label class="sr-only" :for="`submission-ban-${user.user_id}`">
                  제출 밴 분 단위
                </label>
                <input
                  :id="`submission-ban-${user.user_id}`"
                  :value="getDurationDraft(user.user_id)"
                  class="admin-user-ban-input"
                  type="number"
                  min="1"
                  step="1"
                  inputmode="numeric"
                  placeholder="분 단위"
                  :disabled="isBusyUser(user.user_id)"
                  @input="(event) => updateDurationDraft(user.user_id, event.target.value)"
                />
                <button
                  type="button"
                  class="primary-button admin-user-ban-submit"
                  :disabled="isBusyUser(user.user_id)"
                  @click="handleCreateSubmissionBan(user)"
                >
                  {{ getCreateBanButtonLabel(user) }}
                </button>
                <button
                  type="button"
                  class="ghost-button admin-user-ban-clear"
                  :disabled="isBusyUser(user.user_id) || !user.submission_banned_until"
                  @click="handleClearSubmissionBan(user)"
                >
                  {{ getClearBanButtonLabel(user) }}
                </button>
              </div>
            </div>
          </div>
        </div>

        <PaginationBar
          v-if="!isLoading && !errorMessage && filteredUsers.length > pageSize"
          v-model:jump-input="pageJumpInput"
          :current-page="currentPage"
          :total-pages="totalPages"
          :is-loading="isLoading"
          :items="paginationItems"
          jump-input-id="admin-user-management-page-jump"
          :jump-placeholder="`1-${totalPages}`"
          @page-change="goToPage"
          @jump-submit="submitPageJump"
        />
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'

import {
  clearUserSubmissionBan,
  createUserSubmissionBan,
  getUserList,
  getUserSubmissionBan
} from '@/api/user'
import PaginationBar from '@/components/PaginationBar.vue'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'
import { buildPaginationItems } from '@/utils/pagination'

const countFormatter = new Intl.NumberFormat('ko-KR')
const pageSize = 20
const durationPresets = Object.freeze([
  { label: '10분', minutes: 10 },
  { label: '1시간', minutes: 60 },
  { label: '1일', minutes: 1440 },
  { label: '7일', minutes: 10080 },
  { label: '30일', minutes: 43200 }
])

const { authState, isAuthenticated, initializeAuth } = useAuth()
const { showErrorNotice, showSuccessNotice } = useNotice()
const isLoading = ref(true)
const errorMessage = ref('')
const users = ref([])
const searchInput = ref('')
const appliedQuery = ref('')
const currentPage = ref(1)
const pageJumpInput = ref('')
const durationDrafts = ref({})
const actionUserId = ref(0)
const actionType = ref('')
const nowTimestamp = ref(Date.now())
let latestLoadRequestId = 0
let relativeTimeRefreshTimer = null

const canManageUsers = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
const filteredUsers = computed(() => {
  const keyword = appliedQuery.value.trim().toLowerCase()
  if (!keyword) {
    return users.value
  }

  return users.value.filter((user) => {
    return String(user.user_login_id || '').toLowerCase().includes(keyword)
  })
})
const totalPages = computed(() =>
  Math.max(1, Math.ceil(filteredUsers.value.length / pageSize))
)
const pagedUsers = computed(() => {
  const startIndex = (currentPage.value - 1) * pageSize
  return filteredUsers.value.slice(startIndex, startIndex + pageSize)
})
const paginationItems = computed(() =>
  buildPaginationItems(currentPage.value, totalPages.value)
)
const activeBanCount = computed(() =>
  users.value.filter((user) => getSubmissionBanState(user) === 'active').length
)
const normalUserCount = computed(() =>
  users.value.filter((user) => getSubmissionBanState(user) === 'none').length
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
      isLoading.value = false
      return
    }

    loadUsers()
  },
  { immediate: true }
)

watch(currentPage, () => {
  pageJumpInput.value = ''
})

watch(totalPages, (pageCount) => {
  if (currentPage.value > pageCount) {
    currentPage.value = pageCount
  }
})

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
    users.value = await loadSubmissionBanStatuses(
      responseUsers.map(normalizeListUser),
      requestId
    )
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

async function loadSubmissionBanStatuses(baseUsers, requestId){
  if (!authState.token) {
    return baseUsers
  }

  const statusResults = await Promise.all(
    baseUsers.map(async (user) => {
      try {
        const response = await getUserSubmissionBan(user.user_id, authState.token)
        return {
          user_id: user.user_id,
          submission_banned_until:
            typeof response.submission_banned_until === 'string'
              ? response.submission_banned_until
              : null,
          has_error: false
        }
      } catch (error) {
        return {
          user_id: user.user_id,
          submission_banned_until: null,
          has_error: true
        }
      }
    })
  )

  if (requestId !== latestLoadRequestId) {
    return baseUsers
  }

  const statusResultMap = new Map(
    statusResults.map((statusResult) => [statusResult.user_id, statusResult])
  )
  const failedCount = statusResults.filter((statusResult) => statusResult.has_error).length
  if (failedCount > 0) {
    showErrorNotice(
      `${failedCount}명의 제출 밴 상태를 불러오지 못했습니다.`,
      { duration: 5000 }
    )
  }

  return baseUsers.map((user) => {
    const statusResult = statusResultMap.get(user.user_id)
    if (!statusResult) {
      return {
        ...user,
        submission_ban_status_loading: false,
        submission_ban_status_error: true
      }
    }

    const submissionBan = normalizeDateTime(statusResult.submission_banned_until)
    return {
      ...user,
      submission_banned_until: statusResult.submission_banned_until,
      submission_banned_until_timestamp: submissionBan.timestamp,
      submission_banned_until_label: submissionBan.label,
      submission_ban_status_loading: false,
      submission_ban_status_error: statusResult.has_error
    }
  })
}

async function handleCreateSubmissionBan(user){
  if (!authState.token || !canManageUsers.value) {
    showErrorNotice('제출 밴 설정은 관리자만 할 수 있습니다.', { duration: 5000 })
    return
  }

  const durationMinutes = parseDurationDraft(user.user_id)
  if (!durationMinutes) {
    showErrorNotice('제출 밴 시간은 1분 이상의 정수로 입력해 주세요.', { duration: 5000 })
    return
  }

  actionUserId.value = user.user_id
  actionType.value = 'create'

  try {
    const response = await createUserSubmissionBan(
      user.user_id,
      durationMinutes,
      authState.token
    )
    patchUser(user.user_id, {
      submission_banned_until:
        typeof response.submission_banned_until === 'string'
          ? response.submission_banned_until
          : null,
      submission_ban_status_loading: false,
      submission_ban_status_error: false
    })
    durationDrafts.value = {
      ...durationDrafts.value,
      [user.user_id]: ''
    }
    showSuccessNotice(`${user.user_login_id} 님의 제출을 ${durationMinutes}분 동안 제한했습니다.`)
  } catch (error) {
    showErrorNotice(
      error instanceof Error
        ? error.message
        : '제출 밴을 설정하지 못했습니다.',
      { duration: 5000 }
    )
  } finally {
    actionUserId.value = 0
    actionType.value = ''
  }
}

function submitSearch(){
  appliedQuery.value = searchInput.value.trim()
  currentPage.value = 1
}

function resetSearch(){
  searchInput.value = ''
  appliedQuery.value = ''
  currentPage.value = 1
}

function goToPage(pageNumber){
  const clampedPageNumber = Math.min(Math.max(pageNumber, 1), totalPages.value)
  if (clampedPageNumber === currentPage.value) {
    return
  }

  currentPage.value = clampedPageNumber
}

function submitPageJump(){
  const parsedPage = Number.parseInt(pageJumpInput.value, 10)
  if (!Number.isInteger(parsedPage)) {
    pageJumpInput.value = ''
    return
  }

  goToPage(parsedPage)
}

async function handleClearSubmissionBan(user){
  if (!authState.token || !canManageUsers.value) {
    showErrorNotice('제출 밴 해제는 관리자만 할 수 있습니다.', { duration: 5000 })
    return
  }

  actionUserId.value = user.user_id
  actionType.value = 'clear'

  try {
    await clearUserSubmissionBan(user.user_id, authState.token)
    patchUser(user.user_id, {
      submission_banned_until: null,
      submission_ban_status_loading: false,
      submission_ban_status_error: false
    })
    showSuccessNotice(`${user.user_login_id} 님의 제출 제한을 해제했습니다.`)
  } catch (error) {
    showErrorNotice(
      error instanceof Error
        ? error.message
        : '제출 밴을 해제하지 못했습니다.',
      { duration: 5000 }
    )
  } finally {
    actionUserId.value = 0
    actionType.value = ''
  }
}

function patchUser(userId, nextValue){
  users.value = users.value.map((user) => {
    if (user.user_id !== userId) {
      return user
    }

    const submissionBanSource = Object.prototype.hasOwnProperty.call(
      nextValue,
      'submission_banned_until'
    )
      ? nextValue.submission_banned_until
      : user.submission_banned_until
    const normalizedSubmissionBan = normalizeDateTime(submissionBanSource)

    return {
      ...user,
      ...nextValue,
      submission_banned_until:
        typeof submissionBanSource === 'string' ? submissionBanSource : null,
      submission_banned_until_timestamp: normalizedSubmissionBan.timestamp,
      submission_banned_until_label: normalizedSubmissionBan.label
    }
  })
}

function normalizeListUser(user){
  const permissionLevel = normalizePermissionLevel(user.permission_level)
  const createdAt = normalizeDateTime(user.created_at)
  const submissionBan = normalizeDateTime(user.submission_banned_until)

  return {
    user_id: Number(user.user_id ?? 0),
    user_login_id: typeof user.user_login_id === 'string' ? user.user_login_id : '',
    permission_level: permissionLevel,
    role_name: user.role_name || getRoleName(permissionLevel),
    created_at: typeof user.created_at === 'string' ? user.created_at : '',
    created_at_timestamp: createdAt.timestamp,
    created_at_label: createdAt.label,
    submission_banned_until: null,
    submission_banned_until_timestamp: submissionBan.timestamp,
    submission_banned_until_label: submissionBan.label,
    submission_ban_status_loading: true,
    submission_ban_status_error: false
  }
}

function getDurationDraft(userId){
  return durationDrafts.value[userId] ?? ''
}

function setDurationDraft(userId, durationMinutes){
  durationDrafts.value = {
    ...durationDrafts.value,
    [userId]: String(durationMinutes)
  }
}

function updateDurationDraft(userId, value){
  durationDrafts.value = {
    ...durationDrafts.value,
    [userId]: typeof value === 'string' ? value : ''
  }
}

function parseDurationDraft(userId){
  const rawValue = String(getDurationDraft(userId)).trim()
  const durationMinutes = Number(rawValue)
  if (!Number.isInteger(durationMinutes) || durationMinutes <= 0) {
    return 0
  }

  return durationMinutes
}

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
  if (permissionLevel >= 2) {
    return 'danger'
  }

  if (permissionLevel >= 1) {
    return 'warning'
  }

  return 'neutral'
}

function normalizeDateTime(value){
  if (typeof value !== 'string' || !value.trim()) {
    return {
      timestamp: null,
      label: ''
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
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
  return {
    timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    label: trimmedValue
  }
}

function getSubmissionBanState(user){
  if (user.submission_ban_status_loading) {
    return 'loading'
  }

  if (user.submission_ban_status_error) {
    return 'error'
  }

  if (
    typeof user.submission_banned_until_timestamp !== 'number' ||
    Number.isNaN(user.submission_banned_until_timestamp)
  ) {
    return 'none'
  }

  if (user.submission_banned_until_timestamp > nowTimestamp.value) {
    return 'active'
  }

  return 'none'
}

function getSubmissionBanLabel(user){
  const state = getSubmissionBanState(user)
  if (state === 'loading') {
    return '확인 중'
  }

  if (state === 'error') {
    return '조회 실패'
  }

  if (state === 'active') {
    return '제출 금지'
  }

  return '정상'
}

function getSubmissionBanTone(user){
  const state = getSubmissionBanState(user)
  if (state === 'loading') {
    return 'neutral'
  }

  if (state === 'error') {
    return 'warning'
  }

  if (state === 'active') {
    return 'danger'
  }

  return 'success'
}

function formatSubmissionBanWindow(user){
  const state = getSubmissionBanState(user)
  if (state === 'loading') {
    return '상태 확인 중'
  }

  if (state === 'error') {
    return '다시 새로고침해 주세요'
  }

  if (state === 'none') {
    return ''
  }

  const timestamp = user.submission_banned_until_timestamp
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '시각 확인 불가'
  }

  const distance = Math.abs(timestamp - nowTimestamp.value)
  const distanceLabel = formatTimeDistance(distance)

  if (state === 'active') {
    return `${distanceLabel} 남음`
  }

  return ''
}

function shouldShowSubmissionBanUntil(user){
  return Boolean(
    getSubmissionBanState(user) === 'active' &&
    user.submission_banned_until_label
  )
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

  return `${Math.floor(elapsedDays / 365)}년 전`
}

function formatTimeDistance(distanceMs){
  const totalSeconds = Math.max(1, Math.floor(distanceMs / 1000))

  if (totalSeconds < 60) {
    return `${totalSeconds}초`
  }

  const totalMinutes = Math.floor(totalSeconds / 60)
  if (totalMinutes < 60) {
    return `${totalMinutes}분`
  }

  const totalHours = Math.floor(totalMinutes / 60)
  if (totalHours < 24) {
    return `${totalHours}시간`
  }

  const totalDays = Math.floor(totalHours / 24)
  if (totalDays < 30) {
    return `${totalDays}일`
  }

  const totalMonths = Math.floor(totalDays / 30)
  if (totalMonths < 12) {
    return `${totalMonths}달`
  }

  return `${Math.floor(totalDays / 365)}년`
}

function getCreateBanButtonLabel(user){
  if (isBusyUser(user.user_id) && actionType.value === 'create') {
    return '적용 중...'
  }

  return getSubmissionBanState(user) === 'active' ? '기간 갱신' : '밴 설정'
}

function getClearBanButtonLabel(user){
  if (isBusyUser(user.user_id) && actionType.value === 'clear') {
    return '해제 중...'
  }

  return '밴 해제'
}

function isBusyUser(userId){
  return actionUserId.value === userId
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
.admin-user-management-panel {
  display: grid;
  gap: 1rem;
}

.admin-user-management-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-user-management-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
}

.admin-user-management-actions {
  display: flex;
  flex-wrap: wrap;
  justify-content: end;
  align-items: center;
  gap: 0.75rem;
}

.admin-user-search-row {
  display: flex;
  justify-content: flex-end;
  gap: 1rem;
  align-items: center;
  flex-wrap: wrap;
  min-width: 0;
}

.admin-user-search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: 0;
}

.admin-user-search-input {
  width: min(100%, 300px);
  min-height: 2.9rem;
  padding: 0.8rem 0.95rem;
  border-radius: 999px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.admin-user-search-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-user-search-button,
.admin-user-reset-button {
  flex-shrink: 0;
}

.admin-user-management-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.admin-user-management-table {
  border: 1px solid var(--line);
  border-radius: 22px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.72);
}

.admin-user-management-head,
.admin-user-management-row {
  display: grid;
  grid-template-columns: 0.8fr 1.9fr 1.6fr 2.6fr;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.15rem;
}

.admin-user-management-head {
  font-size: 0.88rem;
  font-weight: 700;
  color: var(--ink-soft);
  background: rgba(20, 33, 61, 0.04);
  border-bottom: 1px solid var(--line);
}

.admin-user-management-row + .admin-user-management-row {
  border-top: 1px solid var(--line);
}

.admin-user-management-row {
  content-visibility: auto;
  contain-intrinsic-size: 86px;
}

.admin-user-identity {
  display: grid;
  gap: 0.25rem;
}

.admin-user-link {
  width: fit-content;
  color: inherit;
  text-decoration: none;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.admin-user-link:hover,
.admin-user-link:focus-visible {
  color: #1D4ED8;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.admin-user-created-at {
  white-space: nowrap;
  font-size: 0.92rem;
  font-weight: 600;
  color: #6C8BCF;
}

.admin-user-time-chip {
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

.admin-user-time-chip:hover,
.admin-user-time-chip:focus-visible {
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

.admin-user-time-chip:hover .admin-user-time-tooltip,
.admin-user-time-chip:focus-visible .admin-user-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.admin-user-ban-status {
  display: grid;
  gap: 0.3rem;
}

.admin-user-ban-status :deep(.status-badge) {
  justify-self: start;
  width: fit-content;
  min-height: 1.85rem;
  padding: 0.28rem 0.68rem;
}

.admin-user-ban-copy {
  font-size: 0.94rem;
}

.admin-user-ban-until {
  color: var(--ink-soft);
  font-size: 0.84rem;
}

.admin-user-ban-actions {
  display: grid;
  gap: 0.7rem;
}

.admin-user-ban-presets {
  display: flex;
  flex-wrap: wrap;
  gap: 0.55rem;
}

.admin-user-ban-preset {
  min-height: 2.2rem;
  padding-inline: 0.85rem;
}

.admin-user-ban-form {
  display: flex;
  flex-wrap: wrap;
  gap: 0.6rem;
  align-items: center;
}

.admin-user-ban-input {
  width: 8rem;
  min-height: 2.75rem;
  padding: 0.7rem 0.85rem;
  border: 1px solid rgba(20, 33, 61, 0.12);
  border-radius: 14px;
  background: rgba(255, 255, 255, 0.84);
  color: var(--ink-strong);
}

.admin-user-ban-submit,
.admin-user-ban-clear {
  min-height: 2.75rem;
  padding-inline: 1rem;
  white-space: nowrap;
}

@media (max-width: 1180px) {
  .admin-user-management-toolbar {
    flex-direction: column;
  }

  .admin-user-management-actions {
    justify-content: flex-start;
  }

  .admin-user-search-row,
  .admin-user-search {
    justify-content: flex-start;
  }

  .admin-user-management-summary {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-user-management-table {
    overflow-x: auto;
  }

  .admin-user-management-head,
  .admin-user-management-row {
    min-width: 1120px;
  }
}

@media (max-width: 720px) {
  .admin-user-search-input {
    min-width: 0;
    width: 100%;
  }
}
</style>
