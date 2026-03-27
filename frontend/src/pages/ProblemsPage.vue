<template>
  <section class="page-grid single-column">
    <article class="panel problems-panel">
      <div class="problems-toolbar">
        <div>
          <p class="panel-kicker">problems</p>
          <h3>문제 목록</h3>
        </div>

        <div class="problems-toolbar-actions">
          <div class="problem-summary-group">
            <StatusBadge
              :label="isLoading ? 'Loading' : `${problemCount} Problems`"
              :tone="errorMessage ? 'danger' : 'success'"
            />
            <span
              v-if="!isLoading && problemCount"
              class="problem-summary-text"
            >
              {{ visibleRangeText }}
            </span>
          </div>

          <div class="problem-search-row">
            <form class="problem-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="problem-title-search">문제 제목 검색</label>
              <input
                id="problem-title-search"
                v-model="searchInput"
                class="problem-search-input"
                type="search"
                placeholder="문제 제목 검색"
              />
              <button
                type="submit"
                class="primary-button problem-search-button"
                :disabled="isLoading"
              >
                검색
              </button>
            </form>

            <button
              v-if="hasAppliedTitleFilter"
              type="button"
              class="ghost-button problem-reset-button"
              :disabled="isLoading"
              @click="resetSearch"
            >
              검색 초기화
            </button>
          </div>
        </div>
      </div>

      <div class="problem-filter-bar">
        <div
          v-if="showProblemStateFilters"
          class="problem-filter-group problem-state-filter-group"
        >
          <div class="problem-filter-chip-list">
            <button
              v-for="option in problemStateFilterOptions"
              :key="option.value || 'all'"
              type="button"
              class="ghost-button problem-filter-chip"
              :class="{ 'is-active': appliedStateFilter === option.value }"
              :disabled="isLoading"
              @click="applyStateFilter(option.value)"
            >
              {{ option.label }}
            </button>
          </div>
        </div>

        <div class="problem-filter-group problem-sort-chip-group">
          <span class="problem-filter-label">정렬</span>
          <div class="problem-filter-chip-list">
            <button
              v-for="option in problemSortOptions"
              :key="option.key"
              type="button"
              class="ghost-button problem-filter-chip"
              :class="{ 'is-active': appliedSortKey === option.key }"
              :disabled="isLoading"
              @click="cycleSort(option.key)"
            >
              {{ getSortButtonLabel(option) }}
            </button>
          </div>
        </div>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>문제 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!problemCount" class="empty-state">
        <p>
          {{ emptyStateMessage }}
        </p>
      </div>

      <div v-else class="problem-table">
        <div class="problem-table-head">
          <button
            type="button"
            class="problem-table-head-button problem-table-head-button--number"
            :class="{ 'is-active': appliedSortKey === 'problem_id' }"
            :title="getSortButtonTitle(problemSortOptionMap.problem_id)"
            @click="cycleSort('problem_id')"
          >
            <span class="problem-table-head-button-text">
              {{ problemSortOptionMap.problem_id.label }}
            </span>
            <span class="problem-table-head-button-indicator">
              {{ getSortButtonIndicator(problemSortOptionMap.problem_id.key) }}
            </span>
          </button>
          <span class="problem-table-head-label">제목</span>
          <div class="problem-table-stats-head" aria-label="문제 통계 정렬">
            <button
              type="button"
              class="problem-table-head-button"
              :class="{ 'is-active': appliedSortKey === 'accepted_count' }"
              :title="getSortButtonTitle(problemSortOptionMap.accepted_count)"
              @click="cycleSort('accepted_count')"
            >
              <span class="problem-table-head-button-text">
                {{ problemSortOptionMap.accepted_count.label }}
              </span>
              <span class="problem-table-head-button-indicator">
                {{ getSortButtonIndicator(problemSortOptionMap.accepted_count.key) }}
              </span>
            </button>
            <button
              type="button"
              class="problem-table-head-button"
              :class="{ 'is-active': appliedSortKey === 'submission_count' }"
              :title="getSortButtonTitle(problemSortOptionMap.submission_count)"
              @click="cycleSort('submission_count')"
            >
              <span class="problem-table-head-button-text">
                {{ problemSortOptionMap.submission_count.label }}
              </span>
              <span class="problem-table-head-button-indicator">
                {{ getSortButtonIndicator(problemSortOptionMap.submission_count.key) }}
              </span>
            </button>
            <button
              type="button"
              class="problem-table-head-button"
              :class="{ 'is-active': appliedSortKey === 'acceptance_rate' }"
              :title="getSortButtonTitle(problemSortOptionMap.acceptance_rate)"
              @click="cycleSort('acceptance_rate')"
            >
              <span class="problem-table-head-button-text">
                {{ problemSortOptionMap.acceptance_rate.label }}
              </span>
              <span class="problem-table-head-button-indicator">
                {{ getSortButtonIndicator(problemSortOptionMap.acceptance_rate.key) }}
              </span>
            </button>
          </div>
        </div>

        <RouterLink
          v-for="problem in pagedProblems"
          :key="problem.problem_id"
          class="problem-row problem-row-link"
          :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
        >
          <strong class="problem-id">#{{ problem.problem_id }}</strong>
          <div class="problem-main">
            <div class="problem-title-line">
              <strong>{{ problem.title }}</strong>
              <StatusBadge
                v-if="problem.user_problem_state"
                :label="getProblemStateLabel(problem.user_problem_state)"
                :tone="getProblemStateTone(problem.user_problem_state)"
              />
            </div>
          </div>
          <div class="problem-stats" aria-label="problem statistics">
            <strong class="problem-stat-value">
              {{ formatCount(problem.accepted_count) }}
            </strong>
            <strong class="problem-stat-value">
              {{ formatCount(problem.submission_count) }}
            </strong>
            <strong class="problem-rate-value">
              {{ formatAcceptanceRate(problem.accepted_count, problem.submission_count) }}
            </strong>
          </div>
        </RouterLink>
      </div>

      <div
        v-if="!isLoading && !errorMessage && problemCount > pageSize"
        class="problem-pagination"
      >
        <div class="pagination-controls">
          <button
            type="button"
            class="ghost-button pagination-button"
            :disabled="currentPage === 1"
            @click="goToPage(currentPage - 1)"
          >
            이전
          </button>

          <div class="pagination-pages">
            <template
              v-for="item in paginationItems"
              :key="item.key"
            >
              <button
                v-if="item.type === 'page'"
                type="button"
                class="pagination-page"
                :class="{ 'is-active': item.value === currentPage }"
                @click="goToPage(item.value)"
              >
                {{ item.value }}
              </button>
              <span
                v-else
                class="pagination-ellipsis"
                aria-hidden="true"
              >
                ...
              </span>
            </template>
          </div>

          <button
            type="button"
            class="ghost-button pagination-button"
            :disabled="currentPage === totalPages"
            @click="goToPage(currentPage + 1)"
          >
            다음
          </button>
        </div>

        <form class="pagination-jump" @submit.prevent="submitPageJump">
          <label class="sr-only" for="problem-page-jump">페이지 이동</label>
          <input
            id="problem-page-jump"
            v-model="pageJumpInput"
            class="pagination-jump-input"
            type="number"
            inputmode="numeric"
            min="1"
            :max="totalPages"
            :placeholder="`1-${totalPages}`"
          />
          <button
            type="submit"
            class="ghost-button pagination-jump-button"
            :disabled="isLoading"
          >
            이동
          </button>
        </form>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getProblemList } from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import {
  getProblemStateLabel,
  getProblemStateTone
} from '@/utils/problemState'

const route = useRoute()
const router = useRouter()
const problemSortOptions = [
  {
    key: 'problem_id',
    label: '번호',
    defaultDirection: 'asc'
  },
  {
    key: 'accepted_count',
    label: '정답',
    defaultDirection: 'desc'
  },
  {
    key: 'acceptance_rate',
    label: '정답률',
    defaultDirection: 'desc'
  },
  {
    key: 'submission_count',
    label: '제출',
    defaultDirection: 'desc'
  }
]
const problemStateFilterOptions = [
  { value: '', label: '전체' },
  { value: 'solved', label: '해결' },
  { value: 'unsolved', label: '미해결' }
]
const problemSortOptionMap = Object.fromEntries(
  problemSortOptions.map((option) => [option.key, option])
)
const validProblemSortKeys = new Set(problemSortOptions.map((option) => option.key))
const validProblemStateFilterValues = new Set(problemStateFilterOptions.map((option) => option.value))

const isLoading = ref(true)
const errorMessage = ref('')
const searchInput = ref('')
const problems = ref([])
const pageJumpInput = ref('')
const hasLoadedOnce = ref(false)
const countFormatter = new Intl.NumberFormat()
const rateFormatter = new Intl.NumberFormat('ko-KR', {
  minimumFractionDigits: 1,
  maximumFractionDigits: 1
})
const { authState, isAuthenticated, initializeAuth } = useAuth()

const pageSize = 50
const authenticatedBearerToken = computed(() =>
  authState.initialized && isAuthenticated.value ? authState.token : ''
)
const showProblemStateFilters = computed(() =>
  authState.initialized && isAuthenticated.value
)
const appliedTitleFilter = computed(() => {
  const routeTitle = Array.isArray(route.query.title)
    ? route.query.title[0]
    : route.query.title

  return typeof routeTitle === 'string'
    ? routeTitle.trim()
    : ''
})
const hasAppliedTitleFilter = computed(() => Boolean(appliedTitleFilter.value))
const appliedSortKey = computed(() => {
  const routeSort = Array.isArray(route.query.sort)
    ? route.query.sort[0]
    : route.query.sort

  return normalizeProblemSortKey(routeSort)
})
const appliedSortDirection = computed(() => {
  const routeDirection = Array.isArray(route.query.direction)
    ? route.query.direction[0]
    : route.query.direction

  return normalizeProblemSortDirection(routeDirection, appliedSortKey.value)
})
const appliedStateFilter = computed(() => {
  if (!showProblemStateFilters.value) {
    return ''
  }

  const routeState = Array.isArray(route.query.state)
    ? route.query.state[0]
    : route.query.state

  return normalizeProblemStateFilter(routeState)
})
const currentPage = computed(() => {
  const routePage = Array.isArray(route.query.page)
    ? route.query.page[0]
    : route.query.page

  return normalizeProblemPage(routePage)
})
const filteredProblems = computed(() => {
  const visibleProblems = problems.value.filter((problem) => {
    if (appliedStateFilter.value === 'solved') {
      return problem.user_problem_state === 'solved'
    }

    if (appliedStateFilter.value === 'unsolved') {
      return problem.user_problem_state !== 'solved'
    }

    return true
  })

  return [...visibleProblems].sort(compareProblems)
})
const problemCount = computed(() => filteredProblems.value.length)
const totalPages = computed(() =>
  Math.max(1, Math.ceil(problemCount.value / pageSize))
)
const pagedProblems = computed(() => {
  const startIndex = (currentPage.value - 1) * pageSize
  return filteredProblems.value.slice(startIndex, startIndex + pageSize)
})
const visibleRangeText = computed(() => {
  if (!problemCount.value) {
    return ''
  }

  const start = (currentPage.value - 1) * pageSize + 1
  const end = Math.min(currentPage.value * pageSize, problemCount.value)
  return `${start}-${end} / ${problemCount.value}`
})
const emptyStateMessage = computed(() => {
  if (hasAppliedTitleFilter.value || appliedStateFilter.value) {
    return '조건에 맞는 문제가 없습니다.'
  }

  return '등록된 문제가 아직 없습니다.'
})
const paginationItems = computed(() => {
  const pages = new Set([
    1,
    totalPages.value,
    currentPage.value - 1,
    currentPage.value,
    currentPage.value + 1
  ])

  const sortedPages = Array.from(pages)
    .filter((pageNumber) => pageNumber >= 1 && pageNumber <= totalPages.value)
    .sort((left, right) => left - right)

  const items = []

  sortedPages.forEach((pageNumber, index) => {
    const previousPage = sortedPages[index - 1]

    if (index > 0) {
      if (pageNumber - previousPage === 2) {
        items.push({
          type: 'page',
          value: previousPage + 1,
          key: `page-${previousPage + 1}`
        })
      } else if (pageNumber - previousPage > 2) {
        items.push({
          type: 'ellipsis',
          key: `ellipsis-${previousPage}-${pageNumber}`
        })
      }
    }

    items.push({
      type: 'page',
      value: pageNumber,
      key: `page-${pageNumber}`
    })
  })

  return items
})

let latestLoadRequestId = 0

watch(appliedTitleFilter, (title) => {
  searchInput.value = title
}, {
  immediate: true
})

watch(currentPage, () => {
  pageJumpInput.value = ''
})

watch(totalPages, (pageCount) => {
  if (currentPage.value > pageCount) {
    void replaceProblemBrowseQuery({
      page: pageCount
    })
  }
})

watch(
  [appliedTitleFilter, authenticatedBearerToken],
  ([nextTitle, nextToken], [previousTitle, previousToken]) => {
    if (!hasLoadedOnce.value) {
      return
    }

    if (nextTitle === previousTitle && nextToken === previousToken) {
      return
    }

    loadProblems()
  }
)

watch(showProblemStateFilters, (canShowFilters, couldShowFilters) => {
  if (canShowFilters || !couldShowFilters || !route.query.state) {
    return
  }

  void replaceProblemBrowseQuery({
    stateFilter: '',
    page: 1
  })
})

function normalizeProblemSortKey(rawValue){
  return typeof rawValue === 'string' && validProblemSortKeys.has(rawValue)
    ? rawValue
    : 'problem_id'
}

function getDefaultSortDirection(sortKey){
  return problemSortOptions.find((option) => option.key === sortKey)?.defaultDirection || 'asc'
}

function normalizeProblemSortDirection(rawValue, sortKey){
  if (rawValue === 'asc' || rawValue === 'desc') {
    return rawValue
  }

  return getDefaultSortDirection(sortKey)
}

function normalizeProblemStateFilter(rawValue){
  return typeof rawValue === 'string' && validProblemStateFilterValues.has(rawValue)
    ? rawValue
    : ''
}

function normalizeProblemPage(rawValue){
  const parsedPage = Number.parseInt(rawValue, 10)
  return Number.isInteger(parsedPage) && parsedPage > 0
    ? parsedPage
    : 1
}

function compareAcceptanceRate(left, right){
  if (left.submission_count <= 0 && right.submission_count <= 0) {
    return 0
  }

  if (left.submission_count <= 0) {
    return -1
  }

  if (right.submission_count <= 0) {
    return 1
  }

  const comparisonValue =
    (left.accepted_count * right.submission_count) -
    (right.accepted_count * left.submission_count)

  if (comparisonValue > 0) {
    return 1
  }

  if (comparisonValue < 0) {
    return -1
  }

  return 0
}

function compareProblems(left, right){
  if (appliedSortKey.value === 'problem_id') {
    return appliedSortDirection.value === 'asc'
      ? left.problem_id - right.problem_id
      : right.problem_id - left.problem_id
  }

  if (appliedSortKey.value === 'acceptance_rate') {
    const rateComparison = compareAcceptanceRate(left, right)

    if (rateComparison !== 0) {
      return appliedSortDirection.value === 'asc'
        ? rateComparison
        : -rateComparison
    }

    return left.problem_id - right.problem_id
  }

  if (appliedSortKey.value === 'accepted_count') {
    const acceptedCountDifference = left.accepted_count - right.accepted_count
    if (acceptedCountDifference !== 0) {
      return appliedSortDirection.value === 'asc'
        ? acceptedCountDifference
        : -acceptedCountDifference
    }

    return left.problem_id - right.problem_id
  }

  const submissionCountDifference = left.submission_count - right.submission_count
  if (submissionCountDifference !== 0) {
    return appliedSortDirection.value === 'asc'
      ? submissionCountDifference
      : -submissionCountDifference
  }

  return left.problem_id - right.problem_id
}

function buildProblemBrowseQuery(options = {}){
  const title = options.title ?? appliedTitleFilter.value
  const sortKey = options.sortKey ?? appliedSortKey.value
  const sortDirection = options.sortDirection ?? appliedSortDirection.value
  const stateFilter = options.stateFilter ?? appliedStateFilter.value
  const page = Number(options.page ?? currentPage.value)
  const nextQuery = {}

  if (title) {
    nextQuery.title = title
  }

  if (sortKey !== 'problem_id' || sortDirection !== getDefaultSortDirection('problem_id')) {
    nextQuery.sort = sortKey
  }

  if (sortDirection !== getDefaultSortDirection(sortKey)) {
    nextQuery.direction = sortDirection
  }

  if (showProblemStateFilters.value && stateFilter) {
    nextQuery.state = stateFilter
  }

  if (Number.isInteger(page) && page > 1) {
    nextQuery.page = String(page)
  }

  return nextQuery
}

function areQueryValuesEqual(leftValue, rightValue){
  if (Array.isArray(leftValue) || Array.isArray(rightValue)) {
    const leftValues = Array.isArray(leftValue) ? leftValue : [leftValue]
    const rightValues = Array.isArray(rightValue) ? rightValue : [rightValue]

    if (leftValues.length !== rightValues.length) {
      return false
    }

    return leftValues.every((value, index) => value === rightValues[index])
  }

  return leftValue === rightValue
}

function areQueriesEqual(leftQuery, rightQuery){
  const leftKeys = Object.keys(leftQuery).sort()
  const rightKeys = Object.keys(rightQuery).sort()

  if (leftKeys.length !== rightKeys.length) {
    return false
  }

  return leftKeys.every((key, index) =>
    key === rightKeys[index] && areQueryValuesEqual(leftQuery[key], rightQuery[key])
  )
}

async function replaceProblemBrowseQuery(options = {}){
  const nextQuery = buildProblemBrowseQuery(options)

  if (areQueriesEqual(route.query, nextQuery)) {
    return
  }

  await router.replace({
    name: 'problems',
    query: nextQuery
  })
}

async function loadProblems(){
  const requestId = ++latestLoadRequestId
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getProblemList({
      title: appliedTitleFilter.value,
      bearerToken: authenticatedBearerToken.value
    })

    if (requestId !== latestLoadRequestId) {
      return
    }

    problems.value = Array.isArray(response.problems)
      ? response.problems.map((problem) => ({
        ...problem,
        accepted_count: Number(problem.accepted_count ?? 0),
        submission_count: Number(problem.submission_count ?? 0)
      }))
      : []
    hasLoadedOnce.value = true
  } catch (error) {
    if (requestId !== latestLoadRequestId) {
      return
    }

    errorMessage.value = error instanceof Error
      ? error.message
      : '문제 목록을 불러오지 못했습니다.'
    problems.value = []
    hasLoadedOnce.value = true
  } finally {
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
  }
}

function getSortButtonIndicator(sortKey){
  if (appliedSortKey.value !== sortKey) {
    return '↕'
  }

  return appliedSortDirection.value === 'asc' ? '↑' : '↓'
}

function getSortButtonLabel(option){
  if (appliedSortKey.value !== option.key) {
    return option.label
  }

  return `${option.label} ${appliedSortDirection.value === 'asc' ? '↑' : '↓'}`
}

function getSortButtonTitle(option){
  const nextDirection = appliedSortKey.value === option.key
    ? (appliedSortDirection.value === 'asc' ? 'desc' : 'asc')
    : getDefaultSortDirection(option.key)
  const nextDirectionLabel = nextDirection === 'asc' ? '오름차순' : '내림차순'

  return `${option.label} ${nextDirectionLabel} 정렬`
}

async function cycleSort(sortKey){
  const nextDirection = appliedSortKey.value === sortKey
    ? (appliedSortDirection.value === 'asc' ? 'desc' : 'asc')
    : getDefaultSortDirection(sortKey)

  await replaceProblemBrowseQuery({
    sortKey,
    sortDirection: nextDirection,
    page: 1
  })
}

async function applyStateFilter(stateFilter){
  if (!showProblemStateFilters.value) {
    return
  }

  await replaceProblemBrowseQuery({
    stateFilter,
    page: 1
  })
}

async function submitSearch(){
  await replaceProblemBrowseQuery({
    title: searchInput.value.trim(),
    page: 1
  })
}

async function resetSearch(){
  searchInput.value = ''

  await replaceProblemBrowseQuery({
    title: '',
    page: 1
  })
}

async function goToPage(pageNumber){
  if (pageNumber < 1 || pageNumber > totalPages.value) {
    return
  }

  await replaceProblemBrowseQuery({
    page: pageNumber
  })
}

async function submitPageJump(){
  const parsedPage = Number.parseInt(pageJumpInput.value, 10)

  if (Number.isNaN(parsedPage)) {
    return
  }

  await goToPage(Math.min(Math.max(parsedPage, 1), totalPages.value))
  pageJumpInput.value = ''
}

function formatCount(value){
  return countFormatter.format(value)
}

function formatAcceptanceRate(acceptedCount, submissionCount){
  if (submissionCount <= 0) {
    return '-'
  }

  const rate = (acceptedCount / submissionCount) * 100
  return `${rateFormatter.format(rate)}%`
}

onMounted(async () => {
  if (!authState.initialized) {
    await initializeAuth()
  }

  if (!hasLoadedOnce.value) {
    loadProblems()
  }
})
</script>

<style scoped>
.problems-panel {
  display: grid;
  gap: 1.25rem;
}

.problems-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.problems-toolbar-actions {
  display: flex;
  gap: 1rem;
  align-items: center;
  justify-content: flex-end;
  flex-wrap: wrap;
  margin-left: auto;
  min-width: 0;
}

.problem-summary-group {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-search-row {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
  min-width: 0;
}

.problem-search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: 0;
}

.problem-search-input {
  width: min(100%, 300px);
  min-height: 2.9rem;
  padding: 0.8rem 0.95rem;
  border-radius: 999px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.problem-search-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.problem-search-button,
.problem-reset-button {
  flex-shrink: 0;
}

.problem-summary-text {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
}

.problem-filter-bar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: flex-start;
  flex-wrap: wrap;
  padding-inline: 1.25rem;
}

.problem-filter-group {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-filter-label {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.problem-filter-chip-list {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-filter-chip {
  min-height: 2.55rem;
  padding-inline: 1rem;
}

.problem-filter-chip.is-active {
  color: white;
  border-color: transparent;
  background: linear-gradient(135deg, #d97706, #ea580c);
  box-shadow: 0 16px 30px rgba(217, 119, 6, 0.22);
}

.problem-state-filter-group {
  gap: 0.6rem;
}

.problem-state-filter-group .problem-filter-label {
  line-height: 1;
}

.problem-state-filter-group .problem-filter-chip-list {
  gap: 0.55rem;
}

.problem-state-filter-group .problem-filter-chip {
  min-height: 1.95rem;
  padding-inline: 0.95rem;
  font-size: 0.9rem;
}

.problem-table {
  display: grid;
  border: 1px solid var(--line);
  border-radius: 24px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.78);
  box-shadow:
    0 18px 36px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
}

.problem-sort-chip-group {
  display: none;
}

.problem-table-head,
.problem-row {
  display: grid;
  grid-template-columns: 120px minmax(0, 1fr) 340px;
  gap: 1.15rem;
  align-items: center;
  padding: 1rem 1.25rem;
}

.problem-table-head {
  background: linear-gradient(180deg, rgba(255, 248, 240, 0.96), rgba(255, 255, 255, 0.92));
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.problem-table-head-button {
  display: inline-flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.55rem;
  width: auto;
  max-width: 100%;
  min-height: 2rem;
  padding: 0.3rem 0.75rem;
  border: 1px solid rgba(20, 33, 61, 0.08);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.72);
  color: inherit;
  font: inherit;
  font-weight: inherit;
  letter-spacing: inherit;
  cursor: pointer;
  transition:
    color 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.problem-table-head-button:hover {
  color: var(--ink-strong);
  border-color: rgba(217, 119, 6, 0.22);
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 10px 20px rgba(20, 33, 61, 0.06);
}

.problem-table-head-button.is-active {
  color: var(--ink-strong);
  border-color: rgba(217, 119, 6, 0.28);
  background: rgba(255, 247, 237, 0.96);
  box-shadow:
    0 10px 22px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.78);
}

.problem-table-head-button--number {
  justify-self: start;
  justify-content: flex-start;
  text-align: left;
}

.problem-table-head-button-text {
  min-width: 0;
  white-space: nowrap;
}

.problem-table-head-button-indicator {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 1.25rem;
  min-height: 1.25rem;
  border-radius: 999px;
  background: rgba(20, 33, 61, 0.06);
  color: var(--ink-soft);
  font-size: 0.75rem;
  line-height: 1;
}

.problem-table-head-button.is-active .problem-table-head-button-indicator {
  background: rgba(217, 119, 6, 0.14);
  color: #b45309;
}

.problem-table-head-label {
  color: inherit;
}

.problem-table-stats-head {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 0.9rem;
  align-items: center;
}

.problem-table-stats-head .problem-table-head-button {
  justify-self: end;
  justify-content: flex-end;
  text-align: right;
}

.problem-row {
  background: transparent;
}

.problem-table-head + .problem-row,
.problem-row + .problem-row {
  border-top: 1px solid rgba(20, 33, 61, 0.08);
}

.problem-row-link:hover {
  background: rgba(255, 247, 237, 0.7);
}

.problem-row-link {
  color: inherit;
  transition:
    background 160ms ease,
    color 160ms ease;
}

.problem-id {
  display: block;
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
  padding-left: 0.75rem;
}

.problem-main {
  min-width: 0;
}

.problem-title-line {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-main strong {
  display: block;
}

.problem-stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 0.9rem;
  align-items: center;
  justify-items: stretch;
  text-align: right;
}

.problem-stat-value,
.problem-rate-value {
  display: block;
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
  padding-right: 0.7rem;
}

.problem-rate-value {
  min-width: 4.5rem;
}

.problem-pagination {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
  padding-top: 0.25rem;
}

.pagination-controls {
  display: flex;
  justify-content: center;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.pagination-pages {
  display: flex;
  gap: 0.55rem;
  flex-wrap: wrap;
  justify-content: center;
}

.pagination-button {
  min-width: 88px;
}

.pagination-jump {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  margin-left: auto;
}

.pagination-jump-input {
  width: 7rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.pagination-jump-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.pagination-page {
  min-width: 2.75rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  color: var(--ink-strong);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.pagination-page:hover {
  transform: translateY(-1px);
}

.pagination-page.is-active {
  color: white;
  background: linear-gradient(135deg, #d97706, #ea580c);
  border-color: transparent;
}

.pagination-ellipsis {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 2.75rem;
  min-height: 2.75rem;
  color: var(--ink-soft);
  font-weight: 700;
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

@media (max-width: 900px) {
  .problems-toolbar,
  .problems-toolbar-actions,
  .problem-search-row,
  .problem-search,
  .problem-filter-bar,
  .problem-pagination {
    flex-direction: column;
    align-items: stretch;
  }

  .problems-toolbar-actions {
    width: 100%;
    margin-left: 0;
    justify-content: flex-start;
  }

  .problem-search {
    width: 100%;
  }

  .problem-filter-group {
    align-items: flex-start;
  }

  .problem-sort-chip-group {
    display: flex;
  }

  .pagination-controls,
  .pagination-jump {
    justify-content: center;
    margin-left: 0;
  }

  .pagination-jump-input {
    width: 100%;
  }

  .problem-search-input {
    width: 100%;
  }

  .problem-table-head {
    display: none;
  }

  .problem-row {
    grid-template-columns: minmax(0, 1fr);
  }

  .problem-row-link {
    grid-template-columns: minmax(0, 1fr);
  }

  .problem-stats {
    grid-template-columns: repeat(3, minmax(0, 1fr));
    justify-items: start;
    text-align: left;
  }

  .problem-stat-value,
  .problem-rate-value {
    padding-right: 0;
  }

  .problem-id {
    padding-left: 0;
  }
}
</style>
