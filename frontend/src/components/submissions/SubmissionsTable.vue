<template>
  <div class="submission-table-wrapper">
    <div
      class="submission-table"
      :class="{ 'has-actions': canManageSubmissionRejudge }"
    >
      <div
        class="submission-table-head"
        :class="{ 'has-actions': canManageSubmissionRejudge }"
      >
        <span>제출번호</span>
        <span class="submission-table-head-user">ID</span>
        <span>문제번호</span>
        <span>결과</span>
        <span>실행시간</span>
        <span>메모리</span>
        <span>언어</span>
        <span>제출 시각</span>
        <span v-if="canManageSubmissionRejudge" class="submission-head-action">채점 내역</span>
        <span v-if="canManageSubmissionRejudge" class="submission-head-action">재채점</span>
      </div>

      <div
        v-for="submission in submissions"
        :key="submission.submission_id"
        class="submission-row"
        :class="{ 'has-actions': canManageSubmissionRejudge }"
      >
        <span class="submission-cell is-number">
          {{ formatCount(submission.submission_id) }}
        </span>
        <RouterLink
          class="submission-user-link"
          :to="{ name: 'user-info', params: { userLoginId: submission.user_login_id } }"
        >
          {{ submission.user_login_id }}
        </RouterLink>
        <RouterLink
          class="submission-problem-link"
          :class="getProblemStateTextClass(submission.user_problem_state)"
          :to="{ name: 'problem-detail', params: { problemId: submission.problem_id } }"
        >
          #{{ formatCount(submission.problem_id) }}
        </RouterLink>
        <div class="submission-cell">
          <SubmissionStatusBadge :status="submission.status" />
        </div>
        <span class="submission-cell is-metric">
          {{ formatElapsedMs(submission.elapsed_ms) }}
        </span>
        <span class="submission-cell is-metric">
          {{ formatMemory(submission.max_rss_kb) }}
        </span>
        <button
          v-if="canViewSource(submission)"
          type="button"
          class="submission-language-button"
          @click="$emit('open-source', submission)"
        >
          {{ submission.language }}
        </button>
        <span v-else class="submission-cell is-language">
          {{ submission.language }}
        </span>
        <span class="submission-cell is-submitted-at">
          <span
            class="submission-relative-time"
            tabindex="0"
          >
            {{ formatRelativeSubmittedAt(submission.created_at_timestamp) }}
            <span
              v-if="submission.created_at_label"
              class="submission-time-tooltip"
            >
              {{ submission.created_at_label }}
            </span>
          </span>
        </span>
        <div v-if="canManageSubmissionRejudge" class="submission-cell submission-action-cell">
          <button
            type="button"
            class="ghost-button submission-history-button"
            @click="$emit('open-history', submission)"
          >
            채점 내역
          </button>
        </div>
        <div v-if="canManageSubmissionRejudge" class="submission-cell submission-action-cell">
          <button
            v-if="canRejudgeSubmission(submission)"
            type="button"
            class="ghost-button submission-rejudge-button"
            :disabled="isRejudgingSubmission(submission.submission_id)"
            @click="$emit('rejudge', submission)"
          >
            {{ isRejudgingSubmission(submission.submission_id) ? '요청 중...' : '재채점' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'
import { getProblemStateTextClass } from '@/utils/problemState'

defineProps({
  submissions: {
    type: Array,
    required: true
  },
  canManageSubmissionRejudge: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  formatElapsedMs: {
    type: Function,
    required: true
  },
  formatMemory: {
    type: Function,
    required: true
  },
  formatRelativeSubmittedAt: {
    type: Function,
    required: true
  },
  canViewSource: {
    type: Function,
    required: true
  },
  canRejudgeSubmission: {
    type: Function,
    required: true
  },
  isRejudgingSubmission: {
    type: Function,
    required: true
  }
})

defineEmits(['open-source', 'open-history', 'rejudge'])
</script>

<style scoped>
.submission-table-wrapper {
  overflow-x: auto;
}

.submission-table {
  min-width: 1020px;
  border: 1px solid var(--line);
  border-radius: 1.25rem;
  overflow: visible;
  background: rgba(255, 255, 255, 0.76);
}

.submission-table.has-actions {
  min-width: 1280px;
}

.submission-table-head,
.submission-row {
  display: grid;
  grid-template-columns:
    minmax(88px, 0.9fr)
    minmax(140px, 1.2fr)
    minmax(96px, 0.95fr)
    minmax(120px, 1.1fr)
    minmax(104px, 0.95fr)
    minmax(112px, 1fr)
    minmax(96px, 0.9fr)
    minmax(124px, 1.1fr);
  column-gap: 1rem;
  align-items: center;
  padding: 0.95rem 1.25rem;
}

.submission-table-head.has-actions,
.submission-row.has-actions {
  grid-template-columns:
    minmax(88px, 0.9fr)
    minmax(140px, 1.2fr)
    minmax(96px, 0.95fr)
    minmax(120px, 1.1fr)
    minmax(104px, 0.95fr)
    minmax(112px, 1fr)
    minmax(96px, 0.9fr)
    minmax(124px, 1.1fr)
    minmax(112px, 0.95fr)
    minmax(112px, 0.95fr);
}

.submission-table-head {
  background: rgba(20, 33, 61, 0.06);
  border-bottom: 1px solid var(--line);
  font-size: 0.82rem;
  font-weight: 700;
  color: var(--ink-soft);
}

.submission-table-head-user {
  margin-left: -1.5rem;
}

.submission-row + .submission-row {
  border-top: 1px solid rgba(20, 33, 61, 0.08);
}

.submission-row:hover {
  background: rgba(255, 255, 255, 0.94);
}

.submission-cell {
  min-width: 0;
  color: var(--ink-strong);
  font-variant-numeric: tabular-nums;
}

.submission-cell.is-submitted-at {
  white-space: nowrap;
  font-size: 0.9rem;
  font-weight: 600;
  color: #6C8BCF;
}

.submission-relative-time {
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

.submission-relative-time:hover,
.submission-relative-time:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-time-tooltip {
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

.submission-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.submission-relative-time:hover .submission-time-tooltip,
.submission-relative-time:focus-visible .submission-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.submission-cell.is-number,
.submission-cell.is-user,
.submission-cell.is-metric,
.submission-cell.is-language {
  font-weight: 600;
}

.submission-action-cell {
  display: flex;
  justify-content: center;
}

.submission-head-action {
  justify-self: center;
}

.submission-cell.is-user {
  margin-left: -1.5rem;
}

.submission-user-link {
  min-width: 0;
  margin-left: -1.5rem;
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

.submission-user-link:hover,
.submission-user-link:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-problem-link {
  min-width: 0;
  font-weight: 700;
  color: #6C8BCF;
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

.submission-problem-link:hover,
.submission-problem-link:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-problem-link.problem-state-text--solved {
  color: var(--success);
}

.submission-problem-link.problem-state-text--wrong {
  color: var(--danger);
}

.submission-problem-link.problem-state-text--solved:hover,
.submission-problem-link.problem-state-text--solved:focus-visible {
  color: var(--success);
  text-decoration-color: currentColor;
}

.submission-problem-link.problem-state-text--wrong:hover,
.submission-problem-link.problem-state-text--wrong:focus-visible {
  color: var(--danger);
  text-decoration-color: currentColor;
}

.submission-language-button {
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  min-width: 0;
  padding: 0;
  border: 0;
  background: transparent;
  color: var(--accent);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  text-align: left;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.submission-language-button:hover,
.submission-language-button:focus-visible {
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-rejudge-button,
.submission-history-button {
  min-width: 6rem;
}

.submission-cell.is-language {
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.submission-cell.is-language:hover {
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

@media (max-width: 720px) {
  .submission-table {
    min-width: 980px;
  }

  .submission-table.has-actions {
    min-width: 1240px;
  }
}
</style>
