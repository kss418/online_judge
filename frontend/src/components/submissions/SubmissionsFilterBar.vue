<template>
  <div class="submission-filter-bar">
    <div class="submission-filter-fields">
      <div
        v-if="!hasFixedProblemId"
        class="submission-filter-group"
      >
        <label class="submission-filter-label" for="submission-problem-filter">문제 번호</label>
        <input
          id="submission-problem-filter"
          :value="problemIdFilter"
          class="submission-filter-input"
          type="number"
          inputmode="numeric"
          min="1"
          placeholder="문제 번호"
          :disabled="isLoading"
          @input="$emit('update:problemIdFilter', $event.target.value)"
        />
      </div>

      <div
        v-if="showUserIdFilter"
        class="submission-filter-group"
      >
        <label class="submission-filter-label" for="submission-user-filter">로그인 ID</label>
        <input
          id="submission-user-filter"
          :value="userIdFilter"
          class="submission-filter-input"
          type="text"
          inputmode="text"
          placeholder="예: root"
          :disabled="isLoading"
          @input="$emit('update:userIdFilter', $event.target.value)"
        />
      </div>

      <div class="submission-filter-group">
        <label class="submission-filter-label" for="submission-status-filter">상태 필터</label>
        <select
          id="submission-status-filter"
          :value="statusFilter"
          class="submission-filter-select"
          :disabled="isLoading"
          @change="$emit('update:statusFilter', $event.target.value)"
        >
          <option
            v-for="option in submissionStatusOptions"
            :key="option.value || 'all'"
            :value="option.value"
          >
            {{ option.label }}
          </option>
        </select>
      </div>

      <div class="submission-filter-group">
        <label class="submission-filter-label" for="submission-language-filter">언어 필터</label>
        <select
          id="submission-language-filter"
          :value="languageFilter"
          class="submission-filter-select"
          :disabled="isLoadingLanguages"
          @change="$emit('update:languageFilter', $event.target.value)"
        >
          <option
            v-for="option in submissionLanguageFilterOptions"
            :key="option.value || 'all'"
            :value="option.value"
          >
            {{ option.label }}
          </option>
        </select>
      </div>
    </div>

    <div class="submission-filter-actions">
      <button
        type="button"
        class="ghost-button"
        :disabled="isLoading || !canResetFilters"
        @click="$emit('reset')"
      >
        초기화
      </button>
      <button
        type="button"
        class="primary-button"
        :disabled="isLoading || !canApplyFilters"
        @click="$emit('apply')"
      >
        적용
      </button>
    </div>
  </div>
</template>

<script setup>
defineProps({
  problemIdFilter: {
    type: String,
    default: ''
  },
  userIdFilter: {
    type: String,
    default: ''
  },
  statusFilter: {
    type: String,
    default: ''
  },
  languageFilter: {
    type: String,
    default: ''
  },
  hasFixedProblemId: {
    type: Boolean,
    required: true
  },
  showUserIdFilter: {
    type: Boolean,
    required: true
  },
  isLoading: {
    type: Boolean,
    required: true
  },
  isLoadingLanguages: {
    type: Boolean,
    required: true
  },
  canApplyFilters: {
    type: Boolean,
    required: true
  },
  canResetFilters: {
    type: Boolean,
    required: true
  },
  submissionStatusOptions: {
    type: Array,
    required: true
  },
  submissionLanguageFilterOptions: {
    type: Array,
    required: true
  }
})

defineEmits([
  'update:problemIdFilter',
  'update:userIdFilter',
  'update:statusFilter',
  'update:languageFilter',
  'apply',
  'reset'
])
</script>

<style scoped>
.submission-filter-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 1rem;
  flex-wrap: wrap;
  padding-inline: 1.25rem;
}

.submission-filter-fields {
  display: flex;
  align-items: center;
  gap: 1rem;
  flex-wrap: wrap;
}

.submission-filter-group,
.submission-filter-actions {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-filter-label {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 700;
}

.submission-filter-select {
  min-width: 12rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.95rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.9);
  color: var(--ink-strong);
  font: inherit;
}

.submission-filter-input {
  width: 9rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.95rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.9);
  color: var(--ink-strong);
  font: inherit;
}

.submission-filter-input:focus,
.submission-filter-select:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

@media (max-width: 720px) {
  .submission-filter-fields,
  .submission-filter-bar {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
