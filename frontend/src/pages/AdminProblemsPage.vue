<template>
  <AdminSplitWorkspaceShell
    :access-state="accessState"
    :access-message="accessMessage"
  >
    <template #toolbar>
      <AdminProblemsToolbar
        :toolbar-status-label="toolbarStatusLabel"
        :toolbar-status-tone="toolbarStatusTone"
        :can-manage-problems="canManageProblems"
        :is-loading-problems="isLoadingProblems"
        :busy-section="busySection"
        @refresh="refreshProblems"
      />
    </template>

    <template #sidebar>
      <AdminProblemSelectionSidebar
        v-model:title-search-input="titleSearchInput"
        :search-mode="searchMode"
        :problem-id-search-input="problemIdSearchInput"
        :is-loading-problems="isLoadingProblems"
        :busy-section="busySection"
        :has-applied-search="hasAppliedSearch"
        :problem-list-caption="problemListCaption"
        :problem-count="problemCount"
        :list-error-message="listErrorMessage"
        :empty-problem-list-message="emptyProblemListMessage"
        :problems="problems"
        :selected-problem-id="selectedProblemId"
        :format-count="formatCount"
        :format-problem-limit="formatProblemLimit"
        @set-search-mode="setSearchMode"
        @problem-id-input="handleProblemIdSearchInput"
        @submit-search="submitSearch"
        @reset-search="resetSearch"
        @select-problem="selectProblem"
      >
        <template #create>
          <form class="admin-problems-create" @submit.prevent="handleCreateProblem">
            <div>
              <p class="panel-kicker">create</p>
              <h3>새 문제</h3>
            </div>

            <div class="admin-problems-create-row">
              <input
                v-model="newProblemTitle"
                class="field-input"
                type="text"
                maxlength="120"
                placeholder="새 문제 제목"
              />
            </div>

            <div class="admin-problems-create-actions">
              <button
                type="submit"
                class="primary-button"
                :disabled="!canCreateProblem"
              >
                {{ isCreatingProblem ? '생성 중...' : '문제 생성' }}
              </button>
            </div>
          </form>
        </template>
      </AdminProblemSelectionSidebar>
    </template>

    <AdminProblemsEditor :editor="problemEditor" />
  </AdminSplitWorkspaceShell>

  <AdminProblemConfirmDialog
    v-model:problem-id-input="rejudgeConfirmProblemId"
    v-model:title-input="rejudgeConfirmTitle"
    :open="rejudgeDialogOpen"
    title-id="admin-problem-rejudge-title"
    kicker="confirm rejudge"
    title="문제 재채점 확인"
    description="아래 두 값을 모두 정확히 다시 입력해야 재채점을 요청할 수 있습니다."
    :summary-label="selectedProblemDetail ? `#${formatCount(selectedProblemDetail.problem_id)} ${selectedProblemDetail.title}` : ''"
    :problem-id-placeholder="selectedProblemDetail ? String(selectedProblemDetail.problem_id) : '예: 1000'"
    :title-placeholder="selectedProblemDetail?.title || '문제 제목'"
    summary-copy="현재 문제의 `accepted`, `wrong_answer` 제출을 다시 채점 대기열에 넣습니다."
    :is-busy="isRejudgingProblem"
    :can-confirm="canRejudgeSelectedProblem"
    :confirm-label="isRejudgingProblem ? '요청 중...' : '재채점 확정'"
    confirm-button-class="admin-problem-rejudge-confirm"
    @close="closeRejudgeDialog"
    @confirm="handleRejudgeProblem"
  />

  <AdminProblemConfirmDialog
    v-model:problem-id-input="deleteConfirmProblemId"
    v-model:title-input="deleteConfirmTitle"
    :open="deleteDialogOpen"
    title-id="admin-problem-delete-title"
    kicker="confirm delete"
    title="문제 삭제 확인"
    description="아래 두 값을 모두 정확히 다시 입력해야 삭제할 수 있습니다."
    :summary-label="selectedProblemDetail ? `#${formatCount(selectedProblemDetail.problem_id)} ${selectedProblemDetail.title}` : ''"
    :problem-id-placeholder="selectedProblemDetail ? String(selectedProblemDetail.problem_id) : '예: 1000'"
    :title-placeholder="selectedProblemDetail?.title || '문제 제목'"
    :is-busy="isDeletingProblem"
    :can-confirm="canDeleteSelectedProblem"
    :confirm-label="isDeletingProblem ? '삭제 중...' : '삭제 확정'"
    confirm-button-class="admin-problem-delete-confirm"
    @close="closeDeleteDialog"
    @confirm="handleDeleteProblem"
  />
</template>

<script setup>
import AdminProblemSelectionSidebar from '@/components/adminShared/AdminProblemSelectionSidebar.vue'
import AdminSplitWorkspaceShell from '@/components/adminShared/AdminSplitWorkspaceShell.vue'
import AdminProblemConfirmDialog from '@/components/admin-problems/AdminProblemConfirmDialog.vue'
import AdminProblemsEditor from '@/components/admin-problems/AdminProblemsEditor.vue'
import AdminProblemsToolbar from '@/components/admin-problems/AdminProblemsToolbar.vue'
import { useProblemEditorSections } from '@/composables/adminProblems/useProblemEditorSections'
import { useAdminProblemsPage } from '@/composables/useAdminProblemsPage'

const {
  accessState,
  accessMessage,
  canManageProblems,
  isLoadingProblems,
  isLoadingDetail,
  listErrorMessage,
  detailErrorMessage,
  searchMode,
  titleSearchInput,
  problemIdSearchInput,
  newProblemTitle,
  problems,
  selectedProblemId,
  selectedProblemDetail,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  sampleDrafts,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  busySection,
  rejudgeDialogOpen,
  rejudgeConfirmProblemId,
  rejudgeConfirmTitle,
  deleteDialogOpen,
  deleteConfirmProblemId,
  deleteConfirmTitle,
  problemCount,
  canCreateProblem,
  isCreatingProblem,
  isSavingTitle,
  isSavingLimits,
  isSavingStatement,
  isCreatingSample,
  isDeletingLastSample,
  isUploadingTestcaseZip,
  isRejudgingProblem,
  isDeletingProblem,
  canCreateSample,
  canUploadTestcaseZip,
  canDeleteLastSample,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canDeleteSelectedProblem,
  canRejudgeSelectedProblem,
  hasAppliedSearch,
  problemListCaption,
  emptyProblemListMessage,
  toolbarStatusLabel,
  toolbarStatusTone,
  formatCount,
  formatProblemLimit,
  isSavingSample,
  canSaveSample,
  isLastSample,
  setSearchMode,
  handleProblemIdSearchInput,
  submitSearch,
  resetSearch,
  refreshProblems,
  selectProblem,
  handleCreateProblem,
  handleSaveTitle,
  handleSaveLimits,
  handleSaveStatement,
  handleCreateSample,
  handleSaveSample,
  handleDeleteLastSample,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  openDeleteDialog,
  openRejudgeDialog,
  closeDeleteDialog,
  closeRejudgeDialog,
  handleRejudgeProblem,
  handleDeleteProblem
} = useAdminProblemsPage()

const problemEditor = useProblemEditorSections({
  isLoadingDetail,
  detailErrorMessage,
  selectedProblemDetail,
  busySection,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canCreateSample,
  canUploadTestcaseZip,
  canDeleteLastSample,
  isSavingTitle,
  isSavingLimits,
  isSavingStatement,
  isCreatingSample,
  isDeletingLastSample,
  isUploadingTestcaseZip,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  sampleDrafts,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  formatCount,
  isSavingSample,
  canSaveSample,
  isLastSample,
  handleSaveTitle,
  handleSaveLimits,
  handleSaveStatement,
  handleCreateSample,
  handleSaveSample,
  handleDeleteLastSample,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  openRejudgeDialog,
  openDeleteDialog
})
</script>

<style scoped>
.admin-problems-create {
  display: grid;
  gap: 0.9rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--admin-workspace-section-border);
  background: var(--admin-workspace-section-surface);
  box-shadow: var(--admin-workspace-section-shadow);
}

.admin-problems-create-row {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problems-create-row .field-input {
  flex: 1 1 220px;
}

.admin-problems-create-actions {
  display: flex;
  justify-content: flex-end;
}

@media (max-width: 720px) {
  .admin-problems-create-row {
    display: grid;
    align-items: stretch;
  }
}
</style>
