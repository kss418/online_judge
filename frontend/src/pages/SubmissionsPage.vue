<template>
  <section class="page-grid single-column">
    <article class="panel submissions-panel">
      <SubmissionsToolbar
        :page-title="pageTitle"
        :is-loading="isLoading"
        :error-message="errorMessage"
        :current-submission-count="submissionCount"
        :visible-range-text="visibleRangeText"
        :numeric-problem-id="numericProblemId"
        :format-count="formatCount"
        @refresh="refreshSubmissions"
      />

      <SubmissionsFilterBar
        v-model:problem-id-filter="selectedProblemIdFilter"
        v-model:user-id-filter="selectedUserIdFilter"
        v-model:status-filter="selectedStatusFilter"
        v-model:language-filter="selectedLanguageFilter"
        :has-fixed-problem-id="hasFixedProblemId"
        :show-user-id-filter="showUserIdFilter"
        :is-loading="isLoading"
        :is-loading-languages="isLoadingLanguages"
        :can-apply-filters="canApplyFilters"
        :can-reset-filters="canResetFilters"
        :submission-status-options="submissionStatusOptions"
        :submission-language-filter-options="submissionLanguageFilterOptions"
        @apply="applySubmissionFilters"
        @reset="resetSubmissionFilters"
      />

      <div v-if="isLoading" class="empty-state">
        <p>제출 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!submissionCount" class="empty-state">
        <p>등록된 제출이 아직 없습니다.</p>
      </div>

      <SubmissionsTable
        v-else
        :submissions="submissions"
        :can-manage-submission-rejudge="canManageSubmissionRejudge"
        :format-count="formatCount"
        :format-elapsed-ms="formatElapsedMs"
        :format-memory="formatMemory"
        :format-relative-submitted-at="formatRelativeSubmittedAt"
        :can-view-source="canViewSource"
        :can-rejudge-submission="canRejudgeSubmission"
        :is-rejudging-submission="isRejudgingSubmission"
        @open-source="openSourceDialog"
        @open-history="openHistoryDialog"
        @rejudge="handleRejudgeSubmission"
      />

      <div
        v-if="!isLoading && !errorMessage && (hasPreviousPage || hasMoreSubmissions)"
        class="submission-navigation"
      >
        <button
          type="button"
          class="ghost-button submission-navigation-button"
          :disabled="!hasPreviousPage"
          @click="goToPreviousPage"
        >
          이전
        </button>
        <span class="submission-navigation-label">
          {{ currentPage }} 페이지
        </span>
        <button
          type="button"
          class="ghost-button submission-navigation-button"
          :disabled="!hasMoreSubmissions"
          @click="goToNextPage"
        >
          다음
        </button>
      </div>
    </article>
  </section>

  <SubmissionHistoryDialog
    :open="historyDialogOpen"
    :is-loading="isLoadingHistory"
    :error-message="historyErrorMessage"
    :entries="submissionHistoryEntries"
    :active-submission-id="activeHistorySubmissionId"
    :should-poll="shouldPollSubmissionHistory"
    :format-count="formatCount"
    :format-history-transition="formatHistoryTransition"
    @close="closeHistoryDialog"
  />

  <SubmissionSourceDialog
    :open="sourceDialogOpen"
    :is-loading="isLoadingSource"
    :error-message="sourceErrorMessage"
    :source-detail="sourceDetail"
    :active-submission-id="activeSourceSubmissionId"
    :copy-button-label="copyButtonLabel"
    :format-count="formatCount"
    @close="closeSourceDialog"
    @copy="copySourceCode"
  />
</template>

<script setup>
import SubmissionHistoryDialog from '@/components/submissions/SubmissionHistoryDialog.vue'
import SubmissionSourceDialog from '@/components/submissions/SubmissionSourceDialog.vue'
import SubmissionsFilterBar from '@/components/submissions/SubmissionsFilterBar.vue'
import SubmissionsTable from '@/components/submissions/SubmissionsTable.vue'
import SubmissionsToolbar from '@/components/submissions/SubmissionsToolbar.vue'
import { useSubmissionsPage } from '@/composables/useSubmissionsPage'

const {
  submissionStatusOptions,
  isLoading,
  isLoadingLanguages,
  errorMessage,
  submissions,
  currentPage,
  submissionCount,
  hasPreviousPage,
  hasMoreSubmissions,
  historyDialogOpen,
  isLoadingHistory,
  historyErrorMessage,
  submissionHistoryEntries,
  activeHistorySubmissionId,
  sourceDialogOpen,
  isLoadingSource,
  sourceErrorMessage,
  sourceDetail,
  activeSourceSubmissionId,
  copyButtonLabel,
  selectedProblemIdFilter,
  selectedUserIdFilter,
  selectedStatusFilter,
  selectedLanguageFilter,
  submissionLanguageFilterOptions,
  canManageSubmissionRejudge,
  hasFixedProblemId,
  showUserIdFilter,
  canApplyFilters,
  canResetFilters,
  visibleRangeText,
  numericProblemId,
  shouldPollSubmissionHistory,
  pageTitle,
  formatCount,
  formatRelativeSubmittedAt,
  formatElapsedMs,
  formatMemory,
  formatHistoryTransition,
  canViewSource,
  canRejudgeSubmission,
  isRejudgingSubmission,
  applySubmissionFilters,
  resetSubmissionFilters,
  openHistoryDialog,
  closeHistoryDialog,
  openSourceDialog,
  closeSourceDialog,
  copySourceCode,
  handleRejudgeSubmission,
  refreshSubmissions,
  goToPreviousPage,
  goToNextPage
} = useSubmissionsPage()
</script>

<style scoped>
.submissions-panel {
  display: grid;
  gap: 1.25rem;
}

.submission-navigation {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.9rem;
  flex-wrap: wrap;
}

.submission-navigation-button {
  min-width: 88px;
}

.submission-navigation-label {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 700;
}
</style>
