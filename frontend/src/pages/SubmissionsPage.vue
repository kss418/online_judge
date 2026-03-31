<template>
  <section class="page-grid single-column">
    <article class="panel submissions-panel">
      <SubmissionsToolbar
        :page-title="pageTitle"
        :is-loading="isLoading"
        :error-message="errorMessage"
        :total-submission-count="totalSubmissionCount"
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

      <div v-else-if="!totalSubmissionCount" class="empty-state">
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

      <PaginationBar
        v-if="!isLoading && !errorMessage && totalSubmissionCount > listLimit"
        v-model:jump-input="pageJumpInput"
        :current-page="currentPage"
        :total-pages="totalPages"
        :is-loading="isLoading"
        :items="paginationItems"
        jump-input-id="submission-page-jump"
        :jump-placeholder="`1-${totalPages}`"
        @page-change="goToPage"
        @jump-submit="submitPageJump"
      />
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
import PaginationBar from '@/components/PaginationBar.vue'
import SubmissionHistoryDialog from '@/components/submissions/SubmissionHistoryDialog.vue'
import SubmissionSourceDialog from '@/components/submissions/SubmissionSourceDialog.vue'
import SubmissionsFilterBar from '@/components/submissions/SubmissionsFilterBar.vue'
import SubmissionsTable from '@/components/submissions/SubmissionsTable.vue'
import SubmissionsToolbar from '@/components/submissions/SubmissionsToolbar.vue'
import { useSubmissionsPage } from '@/composables/useSubmissionsPage'

const {
  listLimit,
  submissionStatusOptions,
  isLoading,
  isLoadingLanguages,
  errorMessage,
  submissions,
  currentPage,
  pageJumpInput,
  totalSubmissionCount,
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
  totalPages,
  visibleRangeText,
  paginationItems,
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
  goToPage,
  submitPageJump
} = useSubmissionsPage()
</script>

<style scoped>
.submissions-panel {
  display: grid;
  gap: 1.25rem;
}
</style>
