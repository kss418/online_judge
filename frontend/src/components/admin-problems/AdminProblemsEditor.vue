<template>
  <section class="admin-problem-editor-panel">
    <div v-if="isLoadingDetail" class="empty-state">
      <p>문제 정보를 불러오는 중입니다.</p>
    </div>

    <div v-else-if="detailErrorMessage" class="empty-state error-state">
      <p>{{ detailErrorMessage }}</p>
    </div>

    <div v-else-if="!selectedProblemDetail" class="empty-state">
      <p>왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.</p>
    </div>

    <template v-else>
      <AdminProblemEditorHeader
        :selected-problem-detail="selectedProblemDetail"
        :busy-section="busySection"
        :format-count="formatCount"
        @open-rejudge-dialog="$emit('open-rejudge-dialog')"
      />

      <AdminProblemBasicsSection
        :title-draft="titleDraft"
        :time-limit-draft="timeLimitDraft"
        :memory-limit-draft="memoryLimitDraft"
        :can-save-title="canSaveTitle"
        :can-save-limits="canSaveLimits"
        :is-saving-title="isSavingTitle"
        :is-saving-limits="isSavingLimits"
        @update:title-draft="$emit('update:titleDraft', $event)"
        @update:time-limit-draft="$emit('update:timeLimitDraft', $event)"
        @update:memory-limit-draft="$emit('update:memoryLimitDraft', $event)"
        @save-title="$emit('save-title')"
        @save-limits="$emit('save-limits')"
      />

      <AdminProblemStatementSection
        :description-draft="descriptionDraft"
        :input-format-draft="inputFormatDraft"
        :output-format-draft="outputFormatDraft"
        :note-draft="noteDraft"
        :can-save-statement="canSaveStatement"
        :is-saving-statement="isSavingStatement"
        @update:description-draft="$emit('update:descriptionDraft', $event)"
        @update:input-format-draft="$emit('update:inputFormatDraft', $event)"
        @update:output-format-draft="$emit('update:outputFormatDraft', $event)"
        @update:note-draft="$emit('update:noteDraft', $event)"
        @save-statement="$emit('save-statement')"
      />

      <AdminProblemSamplesSection
        :sample-drafts="sampleDrafts"
        :can-create-sample="canCreateSample"
        :can-delete-last-sample="canDeleteLastSample"
        :is-creating-sample="isCreatingSample"
        :is-deleting-last-sample="isDeletingLastSample"
        :format-count="formatCount"
        :is-saving-sample="isSavingSample"
        :can-save-sample="canSaveSample"
        :is-last-sample="isLastSample"
        @create-sample="$emit('create-sample')"
        @save-sample="$emit('save-sample', $event)"
        @delete-last-sample="$emit('delete-last-sample')"
      />

      <AdminProblemTestcaseUploadSection
        :testcase-zip-input-key="testcaseZipInputKey"
        :selected-testcase-zip-name="selectedTestcaseZipName"
        :busy-section="busySection"
        :can-upload-testcase-zip="canUploadTestcaseZip"
        :is-uploading-testcase-zip="isUploadingTestcaseZip"
        @testcase-zip-change="$emit('testcase-zip-change', $event)"
        @upload-testcase-zip="$emit('upload-testcase-zip')"
      />

      <AdminProblemDangerSection
        :busy-section="busySection"
        @open-delete-dialog="$emit('open-delete-dialog')"
      />
    </template>
  </section>
</template>

<script setup>
import AdminProblemBasicsSection from '@/components/admin-problems/editor/AdminProblemBasicsSection.vue'
import AdminProblemDangerSection from '@/components/admin-problems/editor/AdminProblemDangerSection.vue'
import AdminProblemEditorHeader from '@/components/admin-problems/editor/AdminProblemEditorHeader.vue'
import AdminProblemSamplesSection from '@/components/admin-problems/editor/AdminProblemSamplesSection.vue'
import AdminProblemStatementSection from '@/components/admin-problems/editor/AdminProblemStatementSection.vue'
import AdminProblemTestcaseUploadSection from '@/components/admin-problems/editor/AdminProblemTestcaseUploadSection.vue'

defineProps({
  isLoadingDetail: {
    type: Boolean,
    required: true
  },
  detailErrorMessage: {
    type: String,
    required: true
  },
  selectedProblemDetail: {
    type: Object,
    default: null
  },
  titleDraft: {
    type: String,
    default: ''
  },
  timeLimitDraft: {
    type: String,
    default: ''
  },
  memoryLimitDraft: {
    type: String,
    default: ''
  },
  descriptionDraft: {
    type: String,
    default: ''
  },
  inputFormatDraft: {
    type: String,
    default: ''
  },
  outputFormatDraft: {
    type: String,
    default: ''
  },
  noteDraft: {
    type: String,
    default: ''
  },
  sampleDrafts: {
    type: Array,
    required: true
  },
  testcaseZipInputKey: {
    type: Number,
    required: true
  },
  selectedTestcaseZipName: {
    type: String,
    default: ''
  },
  busySection: {
    type: String,
    default: ''
  },
  canSaveTitle: {
    type: Boolean,
    required: true
  },
  canSaveLimits: {
    type: Boolean,
    required: true
  },
  canSaveStatement: {
    type: Boolean,
    required: true
  },
  canCreateSample: {
    type: Boolean,
    required: true
  },
  canUploadTestcaseZip: {
    type: Boolean,
    required: true
  },
  canDeleteLastSample: {
    type: Boolean,
    required: true
  },
  isSavingTitle: {
    type: Boolean,
    required: true
  },
  isSavingLimits: {
    type: Boolean,
    required: true
  },
  isSavingStatement: {
    type: Boolean,
    required: true
  },
  isCreatingSample: {
    type: Boolean,
    required: true
  },
  isDeletingLastSample: {
    type: Boolean,
    required: true
  },
  isUploadingTestcaseZip: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  isSavingSample: {
    type: Function,
    required: true
  },
  canSaveSample: {
    type: Function,
    required: true
  },
  isLastSample: {
    type: Function,
    required: true
  }
})

defineEmits([
  'update:titleDraft',
  'update:timeLimitDraft',
  'update:memoryLimitDraft',
  'update:descriptionDraft',
  'update:inputFormatDraft',
  'update:outputFormatDraft',
  'update:noteDraft',
  'save-title',
  'save-limits',
  'save-statement',
  'create-sample',
  'save-sample',
  'delete-last-sample',
  'testcase-zip-change',
  'upload-testcase-zip',
  'open-rejudge-dialog',
  'open-delete-dialog'
])
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
