<template>
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
        <form class="problem-search" @submit.prevent="$emit('submitSearch')">
          <label class="sr-only" for="problem-title-search">문제 제목 검색</label>
          <input
            id="problem-title-search"
            :value="searchInput"
            class="problem-search-input"
            type="search"
            placeholder="문제 제목 검색"
            @input="$emit('update:searchInput', $event.target.value)"
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
          @click="$emit('resetSearch')"
        >
          검색 초기화
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  isLoading: {
    type: Boolean,
    default: false
  },
  errorMessage: {
    type: String,
    default: ''
  },
  problemCount: {
    type: Number,
    default: 0
  },
  visibleRangeText: {
    type: String,
    default: ''
  },
  hasAppliedTitleFilter: {
    type: Boolean,
    default: false
  },
  searchInput: {
    type: String,
    default: ''
  }
})

defineEmits([
  'submitSearch',
  'resetSearch',
  'update:searchInput'
])
</script>

<style scoped>
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

.problem-summary-text {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
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
  .problem-search {
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

  .problem-search-input {
    width: 100%;
  }
}
</style>
