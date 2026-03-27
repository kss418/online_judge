<template>
  <aside class="admin-problem-list-panel">
    <form class="admin-problem-search" @submit.prevent="$emit('submit-search')">
      <div class="admin-problem-search-mode-switch" role="tablist" aria-label="문제 검색 방식">
        <button
          type="button"
          class="ghost-button admin-problem-search-mode-button"
          :class="{ 'is-active': searchMode === 'title' }"
          :aria-pressed="searchMode === 'title'"
          @click="$emit('set-search-mode', 'title')"
        >
          제목 검색
        </button>
        <button
          type="button"
          class="ghost-button admin-problem-search-mode-button"
          :class="{ 'is-active': searchMode === 'problem-id' }"
          :aria-pressed="searchMode === 'problem-id'"
          @click="$emit('set-search-mode', 'problem-id')"
        >
          번호 검색
        </button>
      </div>
      <input
        v-if="searchMode === 'title'"
        id="admin-problem-title-search"
        :value="titleSearchInput"
        class="field-input admin-problem-search-input"
        type="search"
        aria-label="문제 제목 검색"
        placeholder="문제 제목 검색"
        @input="$emit('update:titleSearchInput', $event.target.value)"
      />
      <input
        v-else
        id="admin-problem-id-search"
        :value="problemIdSearchInput"
        class="field-input admin-problem-search-input"
        type="text"
        inputmode="numeric"
        aria-label="문제 번호 검색"
        placeholder="예: 123"
        @input="$emit('problem-id-input', $event)"
      />
      <button
        type="submit"
        class="ghost-button"
        :disabled="isLoadingProblems || Boolean(busySection)"
      >
        검색
      </button>
      <button
        v-if="hasAppliedSearch"
        type="button"
        class="ghost-button"
        :disabled="isLoadingProblems || Boolean(busySection)"
        @click="$emit('reset-search')"
      >
        초기화
      </button>
    </form>

    <form class="admin-problem-create" @submit.prevent="$emit('create-problem')">
      <div>
        <p class="panel-kicker">create</p>
        <h3>새 문제</h3>
      </div>
      <div class="admin-problem-create-row">
        <input
          :value="newProblemTitle"
          class="field-input"
          type="text"
          maxlength="120"
          placeholder="새 문제 제목"
          @input="$emit('update:newProblemTitle', $event.target.value)"
        />
      </div>

      <div class="admin-problem-create-actions">
        <button
          type="submit"
          class="primary-button"
          :disabled="!canCreateProblem"
        >
          {{ isCreatingProblem ? '생성 중...' : '문제 생성' }}
        </button>
      </div>
    </form>

    <div class="admin-problem-list-header">
      <p class="admin-problem-list-caption">{{ problemListCaption }}</p>
      <span class="admin-problem-list-count">{{ formatCount(problemCount) }}개</span>
    </div>

    <div v-if="isLoadingProblems" class="empty-state compact-state">
      <p>문제 목록을 불러오는 중입니다.</p>
    </div>

    <div v-else-if="listErrorMessage" class="empty-state error-state compact-state">
      <p>{{ listErrorMessage }}</p>
    </div>

    <div v-else-if="!problemCount" class="empty-state compact-state">
      <p>{{ emptyProblemListMessage }}</p>
    </div>

    <div v-else class="admin-problem-list">
      <button
        v-for="problem in problems"
        :key="problem.problem_id"
        type="button"
        class="admin-problem-item"
        :class="{ 'is-active': problem.problem_id === selectedProblemId }"
        @click="$emit('select-problem', problem.problem_id)"
      >
        <div class="admin-problem-item-head">
          <strong>#{{ formatCount(problem.problem_id) }}</strong>
          <span class="admin-problem-item-version">v{{ problem.version }}</span>
        </div>
        <strong class="admin-problem-item-title">{{ problem.title }}</strong>
        <div class="admin-problem-item-limits">
          <span>{{ formatProblemLimit(problem.time_limit_ms, 'ms') }}</span>
          <span>{{ formatProblemLimit(problem.memory_limit_mb, 'MB') }}</span>
        </div>
      </button>
    </div>
  </aside>
</template>

<script setup>
defineProps({
  searchMode: {
    type: String,
    required: true
  },
  titleSearchInput: {
    type: String,
    default: ''
  },
  problemIdSearchInput: {
    type: String,
    default: ''
  },
  newProblemTitle: {
    type: String,
    default: ''
  },
  isLoadingProblems: {
    type: Boolean,
    required: true
  },
  busySection: {
    type: String,
    default: ''
  },
  hasAppliedSearch: {
    type: Boolean,
    required: true
  },
  canCreateProblem: {
    type: Boolean,
    required: true
  },
  isCreatingProblem: {
    type: Boolean,
    required: true
  },
  problemListCaption: {
    type: String,
    required: true
  },
  problemCount: {
    type: Number,
    required: true
  },
  listErrorMessage: {
    type: String,
    required: true
  },
  emptyProblemListMessage: {
    type: String,
    required: true
  },
  problems: {
    type: Array,
    required: true
  },
  selectedProblemId: {
    type: Number,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  formatProblemLimit: {
    type: Function,
    required: true
  }
})

defineEmits([
  'update:titleSearchInput',
  'update:newProblemTitle',
  'set-search-mode',
  'problem-id-input',
  'submit-search',
  'reset-search',
  'select-problem',
  'create-problem'
])
</script>

<style scoped>
.admin-problem-list-panel {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border: 1px solid var(--admin-problems-shell-border);
  border-radius: 24px;
  background: var(--admin-problems-shell-surface);
  box-shadow: var(--admin-problems-shell-shadow);
}

.admin-problem-search {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-search-mode-switch {
  display: inline-flex;
  gap: 0.5rem;
  padding: 0.3rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.88);
}

.admin-problem-search-mode-button {
  border-radius: 999px;
}

.admin-problem-search-mode-button.is-active {
  color: var(--warning);
  background: rgba(255, 247, 237, 0.98);
  border-color: rgba(217, 119, 6, 0.24);
}

.admin-problem-search-input {
  flex: 1 1 220px;
}

.admin-problem-create {
  display: grid;
  gap: 0.9rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-problem-create-row {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-create-row .field-input {
  flex: 1 1 220px;
}

.admin-problem-create-actions {
  display: flex;
  justify-content: flex-end;
}

.admin-problem-list-header {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-list-caption,
.admin-problem-list-count {
  margin: 0;
  color: var(--ink-soft);
}

.admin-problem-list-count {
  font-size: 0.92rem;
  font-weight: 700;
}

.admin-problem-list {
  display: grid;
  gap: 0.75rem;
  max-height: 65vh;
  overflow-y: auto;
  padding: 0.7rem;
  border: 1px solid var(--admin-problems-section-border);
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.72);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.76),
    inset 0 0 0 1px rgba(255, 255, 255, 0.3);
  scrollbar-gutter: stable;
}

.admin-problem-item {
  appearance: none;
  width: 100%;
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--admin-problems-nested-border);
  background: var(--admin-problems-nested-surface);
  color: inherit;
  text-align: left;
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.admin-problem-item:hover {
  transform: translateY(-1px);
}

.admin-problem-item:hover,
.admin-problem-item.is-active {
  border-color: rgba(217, 119, 6, 0.32);
  background: rgba(255, 251, 235, 0.98);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.12);
}

.admin-problem-item-head,
.admin-problem-item-limits {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-item-head {
  color: var(--ink-soft);
  font-size: 0.86rem;
}

.admin-problem-item-title {
  display: block;
  font-size: 1rem;
  line-height: 1.45;
}

.admin-problem-item-version,
.admin-problem-item-limits {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

@media (max-width: 1100px) {
  .admin-problem-list {
    max-height: 24rem;
  }
}

@media (max-width: 720px) {
  .admin-problem-search,
  .admin-problem-create-row {
    display: grid;
  }

  .admin-problem-item-head,
  .admin-problem-item-limits {
    align-items: start;
    flex-direction: column;
  }
}
</style>
