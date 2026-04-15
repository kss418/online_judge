<template>
  <AdminSelectionSidebarShell
    :list-caption="problemListCaption"
    :item-count="problemCount"
    :is-loading="isLoadingProblems"
    :error-message="listErrorMessage"
    :empty-message="emptyProblemListMessage"
    loading-message="문제 목록을 불러오는 중입니다."
    :format-count="formatCount"
  >
    <template #search>
      <form class="admin-problem-selection-sidebar__search" @submit.prevent="$emit('submit-search')">
        <div
          class="admin-problem-selection-sidebar__mode-switch"
          role="tablist"
          aria-label="문제 검색 방식"
        >
          <button
            type="button"
            class="ghost-button admin-problem-selection-sidebar__mode-button"
            :class="{ 'is-active': searchMode === 'title' }"
            :aria-pressed="searchMode === 'title'"
            @click="$emit('set-search-mode', 'title')"
          >
            제목 검색
          </button>
          <button
            type="button"
            class="ghost-button admin-problem-selection-sidebar__mode-button"
            :class="{ 'is-active': searchMode === 'problem-id' }"
            :aria-pressed="searchMode === 'problem-id'"
            @click="$emit('set-search-mode', 'problem-id')"
          >
            번호 검색
          </button>
        </div>
        <input
          v-if="searchMode === 'title'"
          :id="titleSearchInputId"
          :value="titleSearchInput"
          class="field-input admin-problem-selection-sidebar__search-input"
          type="search"
          aria-label="문제 제목 검색"
          placeholder="문제 제목 검색"
          @input="$emit('update:titleSearchInput', $event.target.value)"
        />
        <input
          v-else
          :id="problemIdSearchInputId"
          :value="problemIdSearchInput"
          class="field-input admin-problem-selection-sidebar__search-input"
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
    </template>

    <template #before-list>
      <slot name="create" />
    </template>

    <button
      v-for="problem in problems"
      :key="problem.problem_id"
      type="button"
      class="admin-problem-selection-sidebar__item"
      :class="{ 'is-active': problem.problem_id === selectedProblemId }"
      @click="$emit('select-problem', problem.problem_id)"
    >
      <div class="admin-problem-selection-sidebar__item-head">
        <strong>#{{ formatCount(problem.problem_id) }}</strong>
        <span class="admin-problem-selection-sidebar__item-version">v{{ problem.version }}</span>
      </div>
      <strong class="admin-problem-selection-sidebar__item-title">{{ problem.title }}</strong>
      <div class="admin-problem-selection-sidebar__item-limits">
        <span>{{ formatProblemLimit(problem.time_limit_ms, 'ms') }}</span>
        <span>{{ formatProblemLimit(problem.memory_limit_mb, 'MB') }}</span>
      </div>
    </button>
  </AdminSelectionSidebarShell>
</template>

<script setup>
import AdminSelectionSidebarShell from '@/components/adminShared/AdminSelectionSidebarShell.vue'

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
  titleSearchInputId: {
    type: String,
    default: 'admin-problem-title-search'
  },
  problemIdSearchInputId: {
    type: String,
    default: 'admin-problem-id-search'
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
  'set-search-mode',
  'problem-id-input',
  'submit-search',
  'reset-search',
  'select-problem'
])
</script>

<style scoped>
.admin-problem-selection-sidebar__search {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-selection-sidebar__mode-switch {
  display: inline-flex;
  gap: 0.5rem;
  padding: 0.3rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.88);
}

.admin-problem-selection-sidebar__mode-button {
  border-radius: 999px;
}

.admin-problem-selection-sidebar__mode-button.is-active {
  color: var(--warning);
  background: rgba(255, 247, 237, 0.98);
  border-color: rgba(217, 119, 6, 0.24);
}

.admin-problem-selection-sidebar__search-input {
  flex: 1 1 220px;
}

.admin-problem-selection-sidebar__item {
  appearance: none;
  width: 100%;
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--admin-workspace-nested-border);
  background: var(--admin-workspace-nested-surface);
  text-align: left;
  font: inherit;
  color: var(--ink-strong);
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.admin-problem-selection-sidebar__item:hover {
  transform: translateY(-1px);
}

.admin-problem-selection-sidebar__item:hover,
.admin-problem-selection-sidebar__item.is-active {
  border-color: rgba(217, 119, 6, 0.32);
  background: rgba(255, 251, 235, 0.98);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.12);
}

.admin-problem-selection-sidebar__item-head,
.admin-problem-selection-sidebar__item-limits {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-selection-sidebar__item-head {
  color: var(--ink-soft);
  font-size: 0.86rem;
}

.admin-problem-selection-sidebar__item-title {
  display: block;
  font-size: 1rem;
  line-height: 1.45;
}

.admin-problem-selection-sidebar__item-version,
.admin-problem-selection-sidebar__item-limits {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

@media (max-width: 720px) {
  .admin-problem-selection-sidebar__search {
    display: grid;
    align-items: stretch;
  }

  .admin-problem-selection-sidebar__item-head,
  .admin-problem-selection-sidebar__item-limits {
    align-items: start;
    flex-direction: column;
  }
}
</style>
