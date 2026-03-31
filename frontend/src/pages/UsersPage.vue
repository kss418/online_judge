<template>
  <section class="page-grid single-column">
    <article class="panel users-panel">
      <div class="users-toolbar">
        <div>
          <p class="panel-kicker">directory</p>
          <h3>유저 목록</h3>
          <p class="users-copy">
            ID로 검색하고, 계정 번호와 제출 기록 요약을 확인할 수 있습니다.
          </p>
        </div>

        <div class="users-toolbar-actions">
          <div class="users-summary-group">
            <StatusBadge
              :label="isLoading ? 'Loading' : `${users.length} Users`"
              :tone="errorMessage ? 'danger' : 'success'"
            />
          </div>

          <div class="users-search-row">
            <form class="users-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="user-search">유저 검색</label>
              <input
                id="user-search"
                v-model.trim="searchInput"
                class="users-search-input"
                type="search"
                placeholder="ID 검색"
              />
              <button
                type="submit"
                class="primary-button users-search-button"
                :disabled="isLoading"
              >
                검색
              </button>
            </form>

            <button
              v-if="appliedQuery"
              type="button"
              class="ghost-button users-reset-button"
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

      <div v-if="isLoading" class="empty-state">
        <p>유저 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!users.length" class="empty-state">
        <p>{{ appliedQuery ? '검색 결과가 없습니다.' : '표시할 유저가 아직 없습니다.' }}</p>
      </div>

      <div v-else class="users-table">
          <div class="users-table-head">
            <span>계정 번호</span>
            <span>ID</span>
            <span>맞은 문제</span>
            <span>제출 수</span>
            <span>정답률</span>
            <span>생성 시각</span>
          </div>

        <div
          v-for="user in pagedUsers"
          :key="user.user_id"
          class="users-row"
        >
          <strong>#{{ countFormatter.format(user.user_id) }}</strong>
          <RouterLink
            class="users-link"
            :to="{ name: 'user-info', params: { userLoginId: user.user_login_id } }"
          >
            {{ user.user_login_id }}
          </RouterLink>
          <strong>{{ countFormatter.format(user.solved_problem_count) }}</strong>
          <strong>{{ countFormatter.format(user.submission_count) }}</strong>
          <strong class="users-acceptance-rate">
            {{ formatAcceptanceRate(user.accepted_submission_count, user.submission_count) }}
          </strong>
          <time
            class="users-created-at"
            :datetime="user.created_at"
          >
            <span class="users-relative-time" tabindex="0">
              {{ formatRelativeCreatedAt(user.created_at_timestamp) }}
              <span
                v-if="user.created_at_label"
                class="users-time-tooltip"
              >
                {{ user.created_at_label }}
              </span>
            </span>
          </time>
        </div>
      </div>

      <PaginationBar
        v-if="!isLoading && !errorMessage && users.length > pageSize"
        v-model:jump-input="pageJumpInput"
        :current-page="currentPage"
        :total-pages="totalPages"
        :is-loading="isLoading"
        :items="paginationItems"
        jump-input-id="users-page-jump"
        :jump-placeholder="`1-${totalPages}`"
        @page-change="goToPage"
        @jump-submit="submitPageJump"
      />
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'

import PaginationBar from '@/components/PaginationBar.vue'
import { getPublicUserList } from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import { buildPaginationItems } from '@/utils/pagination'

const countFormatter = new Intl.NumberFormat('ko-KR')
const rateFormatter = new Intl.NumberFormat('ko-KR', {
  minimumFractionDigits: 1,
  maximumFractionDigits: 1
})
const pageSize = 20
const users = ref([])
const isLoading = ref(true)
const errorMessage = ref('')
const searchInput = ref('')
const appliedQuery = ref('')
const currentPage = ref(1)
const pageJumpInput = ref('')
const nowTimestamp = ref(Date.now())
let relativeTimeRefreshTimer = null

const totalPages = computed(() =>
  Math.max(1, Math.ceil(users.value.length / pageSize))
)
const pagedUsers = computed(() => {
  const startIndex = (currentPage.value - 1) * pageSize
  return users.value.slice(startIndex, startIndex + pageSize)
})
const paginationItems = computed(() =>
  buildPaginationItems(currentPage.value, totalPages.value)
)

function formatAcceptanceRate(acceptedSubmissionCount, submissionCount){
  if (submissionCount <= 0) {
    return '-'
  }

  const rate = (acceptedSubmissionCount / submissionCount) * 100
  return `${rateFormatter.format(rate)}%`
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

  return `${Math.floor(elapsedDays / 365)}년 전`
}

function startRelativeTimeRefresh(){
  nowTimestamp.value = Date.now()
  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
  }

  relativeTimeRefreshTimer = window.setInterval(() => {
    nowTimestamp.value = Date.now()
  }, 30_000)
}

function stopRelativeTimeRefresh(){
  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
    relativeTimeRefreshTimer = null
  }
}

watch(currentPage, () => {
  pageJumpInput.value = ''
})

watch(totalPages, (pageCount) => {
  if (currentPage.value > pageCount) {
    currentPage.value = pageCount
  }
})

async function loadUsers(){
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getPublicUserList(appliedQuery.value)
    const responseUsers = Array.isArray(response.users) ? response.users : []

    users.value = responseUsers.map((user) => ({
      user_id: Number(user.user_id ?? 0),
      user_login_id: typeof user.user_login_id === 'string' ? user.user_login_id : '',
      solved_problem_count: Number(user.solved_problem_count ?? 0),
      accepted_submission_count: Number(user.accepted_submission_count ?? 0),
      submission_count: Number(user.submission_count ?? 0),
      created_at: typeof user.created_at === 'string' ? user.created_at : '',
      ...normalizeCreatedAt(user.created_at)
    }))
  } catch (error) {
    users.value = []
    errorMessage.value = error instanceof Error
      ? error.message
      : '유저 목록을 불러오지 못했습니다.'
  } finally {
    isLoading.value = false
  }
}

function submitSearch(){
  appliedQuery.value = searchInput.value.trim()
  currentPage.value = 1
  void loadUsers()
}

function resetSearch(){
  searchInput.value = ''
  appliedQuery.value = ''
  currentPage.value = 1
  void loadUsers()
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

onMounted(() => {
  startRelativeTimeRefresh()
  void loadUsers()
})

onUnmounted(() => {
  stopRelativeTimeRefresh()
})
</script>

<style scoped>
.users-panel {
  display: grid;
  gap: 1.25rem;
}

.users-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.users-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
  white-space: pre-line;
}

.users-toolbar-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
  align-items: center;
  justify-content: flex-end;
  margin-left: auto;
  min-width: 0;
}

.users-summary-group {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.users-search-row {
  display: flex;
  justify-content: flex-end;
  gap: 1rem;
  align-items: center;
  flex-wrap: wrap;
  min-width: 0;
}

.users-search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: 0;
}

.users-search-input {
  width: min(100%, 300px);
  min-height: 2.9rem;
  padding: 0.8rem 0.95rem;
  border-radius: 999px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.users-search-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.users-table {
  border: 1px solid var(--line);
  border-radius: 22px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.68);
}

.users-table-head,
.users-row {
  display: grid;
  grid-template-columns: 0.8fr 1.5fr 0.8fr 0.8fr 0.85fr 1.2fr;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.15rem;
}

.users-table-head {
  font-size: 0.88rem;
  font-weight: 700;
  color: var(--ink-soft);
  background: rgba(20, 33, 61, 0.04);
  border-bottom: 1px solid var(--line);
}

.users-row + .users-row {
  border-top: 1px solid var(--line);
}

.users-row {
  content-visibility: auto;
  contain-intrinsic-size: 74px;
}

.users-link {
  min-width: 0;
  font-weight: 600;
  color: var(--ink-strong);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.users-link:hover,
.users-link:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.users-search-button,
.users-reset-button {
  flex-shrink: 0;
}

.users-acceptance-rate {
  color: var(--ink-strong);
}

.users-created-at {
  white-space: nowrap;
  font-size: 0.92rem;
  font-weight: 600;
  color: #6C8BCF;
}

.users-relative-time {
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

.users-relative-time:hover,
.users-relative-time:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.users-time-tooltip {
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

.users-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.users-relative-time:hover .users-time-tooltip,
.users-relative-time:focus-visible .users-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

@media (max-width: 1120px) {
  .users-toolbar {
    flex-direction: column;
  }

  .users-toolbar-actions,
  .users-search-row,
  .users-search {
    flex-direction: column;
    justify-content: flex-start;
    align-items: stretch;
  }

  .users-toolbar-actions {
    width: 100%;
    margin-left: 0;
  }

  .users-table {
    overflow-x: auto;
  }

  .users-table-head,
  .users-row {
    min-width: 900px;
  }

  .users-search-input {
    width: 100%;
  }
}
</style>
