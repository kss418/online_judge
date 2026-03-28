<template>
  <section class="page-grid single-column">
    <article class="panel admin-problems-panel">
      <AdminProblemsToolbar
        :toolbar-status-label="toolbarStatusLabel"
        :toolbar-status-tone="toolbarStatusTone"
        :can-manage-problems="canManageProblems"
        :is-loading-problems="isLoadingProblems"
        :busy-section="busySection"
        @refresh="refreshProblems"
      />

      <div v-if="authState.isInitializing" class="empty-state">
        <p>관리자 권한을 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageProblems" class="empty-state error-state">
        <p>이 페이지는 관리자만 접근할 수 있습니다.</p>
      </div>

      <template v-else>
        <div class="admin-problems-layout">
          <AdminProblemsSidebar
            v-model:title-search-input="titleSearchInput"
            v-model:new-problem-title="newProblemTitle"
            :search-mode="searchMode"
            :problem-id-search-input="problemIdSearchInput"
            :is-loading-problems="isLoadingProblems"
            :busy-section="busySection"
            :has-applied-search="hasAppliedSearch"
            :can-create-problem="canCreateProblem"
            :is-creating-problem="isCreatingProblem"
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
            @create-problem="handleCreateProblem"
          />

          <AdminProblemsEditor
            v-model:title-draft="titleDraft"
            v-model:time-limit-draft="timeLimitDraft"
            v-model:memory-limit-draft="memoryLimitDraft"
            v-model:description-draft="descriptionDraft"
            v-model:input-format-draft="inputFormatDraft"
            v-model:output-format-draft="outputFormatDraft"
            v-model:note-draft="noteDraft"
            :is-loading-detail="isLoadingDetail"
            :detail-error-message="detailErrorMessage"
            :selected-problem-detail="selectedProblemDetail"
            :sample-drafts="sampleDrafts"
            :testcase-zip-input-key="testcaseZipInputKey"
            :selected-testcase-zip-name="selectedTestcaseZipName"
            :busy-section="busySection"
            :can-save-title="canSaveTitle"
            :can-save-limits="canSaveLimits"
            :can-save-statement="canSaveStatement"
            :can-create-sample="canCreateSample"
            :can-upload-testcase-zip="canUploadTestcaseZip"
            :can-delete-last-sample="canDeleteLastSample"
            :is-saving-title="isSavingTitle"
            :is-saving-limits="isSavingLimits"
            :is-saving-statement="isSavingStatement"
            :is-creating-sample="isCreatingSample"
            :is-deleting-last-sample="isDeletingLastSample"
            :is-uploading-testcase-zip="isUploadingTestcaseZip"
            :format-count="formatCount"
            :is-saving-sample="isSavingSample"
            :can-save-sample="canSaveSample"
            :is-last-sample="isLastSample"
            @save-title="handleSaveTitle"
            @save-limits="handleSaveLimits"
            @save-statement="handleSaveStatement"
            @create-sample="handleCreateSample"
            @save-sample="handleSaveSample"
            @delete-last-sample="handleDeleteLastSample"
            @testcase-zip-change="handleTestcaseZipFileChange"
            @upload-testcase-zip="handleUploadTestcaseZip"
            @open-rejudge-dialog="openRejudgeDialog"
            @open-delete-dialog="openDeleteDialog"
          />
        </div>
      </template>
    </article>
  </section>

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
import AdminProblemConfirmDialog from '@/components/admin-problems/AdminProblemConfirmDialog.vue'
import AdminProblemsEditor from '@/components/admin-problems/AdminProblemsEditor.vue'
import AdminProblemsSidebar from '@/components/admin-problems/AdminProblemsSidebar.vue'
import AdminProblemsToolbar from '@/components/admin-problems/AdminProblemsToolbar.vue'
import { useAdminProblemsPage } from '@/composables/useAdminProblemsPage'

const {
  authState,
  isAuthenticated,
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
</script>

<style scoped>
.admin-problems-panel {
  display: grid;
  gap: 1rem;
  --admin-problems-shell-surface: linear-gradient(
    180deg,
    rgba(246, 248, 251, 0.98),
    rgba(239, 243, 248, 0.94)
  );
  --admin-problems-shell-border: rgba(148, 163, 184, 0.18);
  --admin-problems-section-surface: linear-gradient(
    180deg,
    rgba(255, 255, 255, 0.98),
    rgba(248, 250, 252, 0.95)
  );
  --admin-problems-section-border: rgba(148, 163, 184, 0.12);
  --admin-problems-nested-surface: rgba(255, 255, 255, 0.98);
  --admin-problems-nested-border: rgba(148, 163, 184, 0.14);
  --admin-problems-shell-shadow:
    0 18px 36px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.7);
  --admin-problems-section-shadow:
    0 12px 28px rgba(20, 33, 61, 0.04),
    inset 0 1px 0 rgba(255, 255, 255, 0.76);
}

.admin-problems-layout {
  display: grid;
  grid-template-columns: minmax(320px, 380px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

@media (max-width: 1100px) {
  .admin-problems-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
