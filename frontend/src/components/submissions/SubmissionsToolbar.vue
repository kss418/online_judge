<template>
  <div class="submissions-toolbar">
    <div>
      <p class="panel-kicker">submissions</p>
      <h3>{{ pageTitle }}</h3>
    </div>

    <div class="submissions-toolbar-actions">
      <div class="submission-summary-group">
        <StatusBadge
          :label="isLoading ? 'Loading' : `${formatCount(currentSubmissionCount)} submissions`"
          :tone="errorMessage ? 'danger' : 'success'"
        />
        <span
          v-if="!isLoading && currentSubmissionCount"
          class="submission-summary-text"
        >
          {{ visibleRangeText }}
        </span>
      </div>

      <button
        type="button"
        class="ghost-button submissions-refresh-button"
        :disabled="isLoading"
        @click="$emit('refresh')"
      >
        새로고침
      </button>

      <RouterLink
        v-if="numericProblemId"
        class="submissions-back-link"
        :to="{ name: 'problem-detail', params: { problemId: numericProblemId } }"
      >
        문제로 돌아가기
      </RouterLink>
    </div>
  </div>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  pageTitle: {
    type: String,
    required: true
  },
  isLoading: {
    type: Boolean,
    required: true
  },
  errorMessage: {
    type: String,
    required: true
  },
  currentSubmissionCount: {
    type: Number,
    required: true
  },
  visibleRangeText: {
    type: String,
    default: ''
  },
  numericProblemId: {
    type: Number,
    default: null
  },
  formatCount: {
    type: Function,
    required: true
  }
})

defineEmits(['refresh'])
</script>

<style scoped>
.submissions-back-link {
  display: inline-flex;
  align-items: center;
  min-height: 2.6rem;
  padding: 0.6rem 1rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  font-weight: 700;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.submissions-refresh-button {
  flex-shrink: 0;
}

.submissions-back-link:hover {
  transform: translateY(-1px);
  border-color: rgba(20, 33, 61, 0.24);
}

.submissions-toolbar {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 1rem;
  flex-wrap: wrap;
}

.submissions-toolbar-actions {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 0.75rem;
  flex-wrap: wrap;
  margin-left: auto;
}

.submission-summary-group {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-summary-text {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
}

@media (max-width: 720px) {
  .submissions-toolbar,
  .submissions-toolbar-actions {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
