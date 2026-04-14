<template>
  <article class="admin-problem-editor-section admin-problem-editor-section--testcases">
    <div class="panel-header admin-sample-section-header">
      <div>
        <p class="panel-kicker">testcases</p>
        <h3>테스트케이스</h3>
        <p class="admin-problem-section-copy">
          ZIP 업로드 시 기존 테스트케이스는 모두 교체됩니다. `001.in` / `001.out`부터
          `999.in` / `999.out`까지 연속 순번만 허용하며, 빈 파일은 허용됩니다.
        </p>
      </div>
    </div>

    <div class="admin-testcase-upload-panel">
      <label class="field-block">
        <span class="field-label">ZIP 파일</span>
        <input
          :key="section.model.testcaseZipInputKey"
          class="admin-testcase-file-input"
          type="file"
          accept=".zip,application/zip"
          :disabled="Boolean(section.model.busySection)"
          @change="handleTestcaseZipChange"
        />
      </label>

      <p v-if="section.model.selectedTestcaseZipName" class="admin-testcase-selected-file">
        선택한 파일: {{ section.model.selectedTestcaseZipName }}
      </p>
    </div>

    <div class="admin-problem-section-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!section.model.canUploadTestcaseZip"
        @click="handleUploadTestcaseZip"
      >
        {{ section.model.isUploadingTestcaseZip ? '업로드 중...' : 'ZIP 업로드' }}
      </button>
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleTestcaseZipChange(event){
  section.value.actions.changeTestcaseZip(event)
}

function handleUploadTestcaseZip(){
  section.value.actions.uploadTestcaseZip()
}
</script>

<style scoped>
.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-sample-section-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-section-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-testcase-upload-panel {
  display: grid;
  gap: 0.75rem;
  padding: 0.95rem 1rem;
  border: 1px dashed rgba(148, 163, 184, 0.24);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.72);
}

.admin-testcase-file-input {
  display: block;
  width: 100%;
  padding: 0.85rem 1rem;
  border-radius: 18px;
  border: 1px dashed rgba(20, 33, 61, 0.18);
  background: var(--admin-problems-nested-surface);
  color: var(--ink-strong);
  font: inherit;
}

.admin-testcase-selected-file {
  margin: 0;
  color: var(--ink-soft);
  word-break: break-all;
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}
</style>
