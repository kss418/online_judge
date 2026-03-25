<template>
  <section class="page-grid single-column">
    <article class="panel problems-panel">
      <div class="problems-toolbar">
        <div>
          <p class="panel-kicker">problems</p>
          <h3>문제 목록</h3>
          <p class="lead-copy problems-copy">
            등록된 문제를 최신 순으로 보여줍니다.
          </p>
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
        <StatusBadge
          :label="isLoading ? 'Loading' : `${problemCount} Problems`"
          :tone="errorMessage ? 'danger' : 'success'"
        />
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
          <span>버전</span>
        </div>

        <div
          v-for="problem in problems"
          :key="problem.problem_id"
          class="problem-row"
        >
          <strong class="problem-id">#{{ problem.problem_id }}</strong>
          <div class="problem-main">
            <strong>{{ problem.title }}</strong>
            <span class="problem-meta">문제 상세와 제출 화면을 다음 단계에서 연결할 수 있습니다.</span>
          </div>
          <StatusBadge
            :label="`v${problem.version}`"
            tone="neutral"
          />
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'

import { getProblemList } from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'

const isLoading = ref(true)
const errorMessage = ref('')
const searchInput = ref('')
const searchKeyword = ref('')
const problems = ref([])

const problemCount = computed(() => problems.value.length)

async function loadProblems(){
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getProblemList({
      title: searchKeyword.value
    })

    problems.value = Array.isArray(response.problems)
      ? response.problems
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
  searchKeyword.value = searchInput.value.trim()
  loadProblems()
}

function resetSearch(){
  searchInput.value = ''
  searchKeyword.value = ''
  loadProblems()
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

.problem-table {
  display: grid;
  gap: 0.8rem;
}

.problem-table-head,
.problem-row {
  display: grid;
  grid-template-columns: 120px minmax(0, 1fr) 110px;
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

.problem-meta {
  display: block;
  color: var(--ink-soft);
  font-size: 0.92rem;
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
  .problem-summary-bar {
    flex-direction: column;
    align-items: stretch;
  }

  .problem-search {
    min-width: 0;
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
}
</style>
