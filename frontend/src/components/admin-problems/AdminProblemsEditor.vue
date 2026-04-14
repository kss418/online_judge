<template>
  <section class="admin-problem-editor-panel">
    <div v-if="editorState.isLoadingDetail" class="empty-state">
      <p>문제 정보를 불러오는 중입니다.</p>
    </div>

    <div v-else-if="editorState.detailErrorMessage" class="empty-state error-state">
      <p>{{ editorState.detailErrorMessage }}</p>
    </div>

    <div v-else-if="!editorState.selectedProblemDetail" class="empty-state">
      <p>왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.</p>
    </div>

    <template v-else>
      <AdminProblemEditorHeader :section="headerSection" />

      <AdminProblemBasicsSection :section="basicsSection" />

      <AdminProblemStatementSection :section="statementSection" />

      <AdminProblemSamplesSection :section="samplesSection" />

      <AdminProblemTestcaseUploadSection :section="testcaseUploadSection" />

      <AdminProblemDangerSection :section="dangerSection" />
    </template>
  </section>
</template>

<script setup>
import { computed } from 'vue'

import AdminProblemBasicsSection from '@/components/admin-problems/editor/AdminProblemBasicsSection.vue'
import AdminProblemDangerSection from '@/components/admin-problems/editor/AdminProblemDangerSection.vue'
import AdminProblemEditorHeader from '@/components/admin-problems/editor/AdminProblemEditorHeader.vue'
import AdminProblemSamplesSection from '@/components/admin-problems/editor/AdminProblemSamplesSection.vue'
import AdminProblemStatementSection from '@/components/admin-problems/editor/AdminProblemStatementSection.vue'
import AdminProblemTestcaseUploadSection from '@/components/admin-problems/editor/AdminProblemTestcaseUploadSection.vue'

const props = defineProps({
  editorState: {
    type: Object,
    required: true
  },
  editorDraft: {
    type: Object,
    required: true
  },
  editorActions: {
    type: Object,
    required: true
  }
})

const headerSection = computed(() => ({
  model: {
    selectedProblemDetail: props.editorState.selectedProblemDetail,
    busySection: props.editorState.busySection,
    formatCount: props.editorState.formatCount
  },
  actions: {
    openRejudgeDialog: props.editorActions.openRejudgeDialog
  }
}))

const basicsSection = computed(() => ({
  model: {
    titleDraft: props.editorDraft.titleDraft,
    timeLimitDraft: props.editorDraft.timeLimitDraft,
    memoryLimitDraft: props.editorDraft.memoryLimitDraft,
    canSaveTitle: props.editorState.canSaveTitle,
    canSaveLimits: props.editorState.canSaveLimits,
    isSavingTitle: props.editorState.isSavingTitle,
    isSavingLimits: props.editorState.isSavingLimits
  },
  actions: {
    updateTitleDraft: props.editorActions.updateTitleDraft,
    updateTimeLimitDraft: props.editorActions.updateTimeLimitDraft,
    updateMemoryLimitDraft: props.editorActions.updateMemoryLimitDraft,
    saveTitle: props.editorActions.saveTitle,
    saveLimits: props.editorActions.saveLimits
  }
}))

const statementSection = computed(() => ({
  model: {
    descriptionDraft: props.editorDraft.descriptionDraft,
    inputFormatDraft: props.editorDraft.inputFormatDraft,
    outputFormatDraft: props.editorDraft.outputFormatDraft,
    noteDraft: props.editorDraft.noteDraft,
    canSaveStatement: props.editorState.canSaveStatement,
    isSavingStatement: props.editorState.isSavingStatement
  },
  actions: {
    updateDescriptionDraft: props.editorActions.updateDescriptionDraft,
    updateInputFormatDraft: props.editorActions.updateInputFormatDraft,
    updateOutputFormatDraft: props.editorActions.updateOutputFormatDraft,
    updateNoteDraft: props.editorActions.updateNoteDraft,
    saveStatement: props.editorActions.saveStatement
  }
}))

const samplesSection = computed(() => ({
  model: {
    sampleDrafts: props.editorDraft.sampleDrafts,
    canCreateSample: props.editorState.canCreateSample,
    canDeleteLastSample: props.editorState.canDeleteLastSample,
    isCreatingSample: props.editorState.isCreatingSample,
    isDeletingLastSample: props.editorState.isDeletingLastSample,
    formatCount: props.editorState.formatCount,
    isSavingSample: props.editorState.isSavingSample,
    canSaveSample: props.editorState.canSaveSample,
    isLastSample: props.editorState.isLastSample
  },
  actions: {
    createSample: props.editorActions.createSample,
    saveSample: props.editorActions.saveSample,
    deleteLastSample: props.editorActions.deleteLastSample
  }
}))

const testcaseUploadSection = computed(() => ({
  model: {
    testcaseZipInputKey: props.editorDraft.testcaseZipInputKey,
    selectedTestcaseZipName: props.editorDraft.selectedTestcaseZipName,
    busySection: props.editorState.busySection,
    canUploadTestcaseZip: props.editorState.canUploadTestcaseZip,
    isUploadingTestcaseZip: props.editorState.isUploadingTestcaseZip
  },
  actions: {
    changeTestcaseZip: props.editorActions.changeTestcaseZip,
    uploadTestcaseZip: props.editorActions.uploadTestcaseZip
  }
}))

const dangerSection = computed(() => ({
  model: {
    busySection: props.editorState.busySection
  },
  actions: {
    openDeleteDialog: props.editorActions.openDeleteDialog
  }
}))
</script>

<style scoped>
.admin-problem-editor-panel {
  display: grid;
  gap: 1rem;
  min-height: 40rem;
  padding: 1rem;
  border: 1px solid var(--admin-problems-shell-border);
  border-radius: 24px;
  background: var(--admin-problems-shell-surface);
  box-shadow: var(--admin-problems-shell-shadow);
}
</style>
