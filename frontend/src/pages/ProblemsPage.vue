<template>
  <section class="page-grid single-column">
    <article class="panel problems-panel">
      <div class="problems-toolbar">
        <div>
          <p class="panel-kicker">problems</p>
          <h3>문제 목록</h3>
        </div>

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
      </div>

      <div class="problem-summary-bar">
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

        <button
          v-if="searchKeyword"
          type="button"
          class="ghost-button problem-reset-button"
          :disabled="isLoading"
          @click="resetSearch"
        >
          검색 초기화
        </button>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>문제 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!problemCount" class="empty-state">
        <p>
          {{ searchKeyword ? '검색 조건에 맞는 문제가 없습니다.' : '등록된 문제가 아직 없습니다.' }}
        </p>
      </div>

      <div v-else class="problem-table">
        <div class="problem-table-head">
          <span>번호</span>
          <span>제목</span>
          <span>통계</span>
        </div>

        <RouterLink
          v-for="problem in pagedProblems"
          :key="problem.problem_id"
          class="problem-row problem-row-link"
          :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
        >
          <strong class="problem-id">#{{ problem.problem_id }}</strong>
          <div class="problem-main">
            <strong>{{ problem.title }}</strong>
          </div>
          <div class="problem-stats" aria-label="problem statistics">
            <div class="problem-counts">
              <div class="problem-stat">
                <span class="problem-stat-label">정답</span>
                <strong>{{ formatCount(problem.accepted_count) }}</strong>
              </div>
              <div class="problem-stat">
                <span class="problem-stat-label">제출</span>
                <strong>{{ formatCount(problem.submission_count) }}</strong>
              </div>
            </div>
            <div class="problem-rate">
              <span class="problem-rate-label">정답률</span>
              <strong class="problem-rate-value">
                {{ formatAcceptanceRate(problem.accepted_count, problem.submission_count) }}
              </strong>
            </div>
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

import { getProblemList } from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'

const isLoading = ref(true)
const errorMessage = ref('')
const searchInput = ref('')
const searchKeyword = ref('')
const problems = ref([])
const currentPage = ref(1)
const pageJumpInput = ref('')
const countFormatter = new Intl.NumberFormat()
const rateFormatter = new Intl.NumberFormat('ko-KR', {
  minimumFractionDigits: 1,
  maximumFractionDigits: 1
})

const pageSize = 50

const problemCount = computed(() => problems.value.length)
const sortedProblems = computed(() =>
  [...problems.value].sort((left, right) => left.problem_id - right.problem_id)
)
const totalPages = computed(() =>
  Math.max(1, Math.ceil(problemCount.value / pageSize))
)
const pagedProblems = computed(() => {
  const startIndex = (currentPage.value - 1) * pageSize
  return sortedProblems.value.slice(startIndex, startIndex + pageSize)
})
const visibleRangeText = computed(() => {
  if (!problemCount.value) {
    return ''
  }

  const start = (currentPage.value - 1) * pageSize + 1
  const end = Math.min(currentPage.value * pageSize, problemCount.value)
  return `${start}-${end} / ${problemCount.value}`
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

watch(totalPages, (pageCount) => {
  if (currentPage.value > pageCount) {
    currentPage.value = pageCount
  }
})

async function loadProblems(){
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getProblemList({
      title: searchKeyword.value
    })

    problems.value = Array.isArray(response.problems)
      ? response.problems.map((problem) => ({
        ...problem,
        accepted_count: Number(problem.accepted_count ?? 0),
        submission_count: Number(problem.submission_count ?? 0)
      }))
      : []
  } catch (error) {
    errorMessage.value = error instanceof Error
      ? error.message
      : '문제 목록을 불러오지 못했습니다.'
    problems.value = []
  } finally {
    isLoading.value = false
  }
}

function submitSearch(){
  currentPage.value = 1
  searchKeyword.value = searchInput.value.trim()
  loadProblems()
}

function resetSearch(){
  currentPage.value = 1
  searchInput.value = ''
  searchKeyword.value = ''
  loadProblems()
}

function goToPage(pageNumber){
  if (pageNumber < 1 || pageNumber > totalPages.value) {
    return
  }

  currentPage.value = pageNumber
}

function submitPageJump(){
  const parsedPage = Number.parseInt(pageJumpInput.value, 10)

  if (Number.isNaN(parsedPage)) {
    return
  }

  goToPage(Math.min(Math.max(parsedPage, 1), totalPages.value))
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

onMounted(() => {
  loadProblems()
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

.problems-copy {
  margin-top: 0.5rem;
  margin-bottom: 0;
}

.problem-search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: min(100%, 420px);
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

.problem-summary-bar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: center;
}

.problem-summary-group {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-summary-text {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
}

.problem-table {
  display: grid;
  gap: 0.8rem;
}

.problem-table-head,
.problem-row {
  display: grid;
  grid-template-columns: 120px minmax(0, 1fr) 280px;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.1rem;
  border-radius: 20px;
}

.problem-table-head {
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.problem-row {
  border: 1px solid var(--line);
  background: var(--surface-strong);
}

.problem-row-link:hover {
  transform: translateY(-1px);
}

.problem-row-link {
  color: inherit;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.problem-id {
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
}

.problem-main {
  min-width: 0;
}

.problem-main strong {
  display: block;
  margin-bottom: 0.2rem;
}

.problem-stats {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 1rem;
  align-items: center;
}

.problem-counts {
  display: grid;
  gap: 0.45rem;
  justify-items: start;
}

.problem-stat-label {
  color: var(--ink-soft);
  font-size: 0.82rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  font-weight: 700;
}

.problem-stat {
  display: flex;
  gap: 0.5rem;
  align-items: baseline;
}

.problem-rate {
  display: grid;
  gap: 0.2rem;
  justify-items: end;
  text-align: right;
}

.problem-rate-label {
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.problem-rate-value {
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
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
  .problem-search,
  .problem-summary-bar,
  .problem-pagination {
    flex-direction: column;
    align-items: stretch;
  }

  .problem-search {
    min-width: 0;
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
    grid-template-columns: minmax(0, 1fr);
    justify-items: start;
  }

  .problem-rate {
    justify-items: start;
    text-align: left;
  }
}
</style>
