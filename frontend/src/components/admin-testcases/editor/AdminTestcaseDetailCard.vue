<template>
  <article
    v-if="section.model.isLoadingTestcases"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스 요약을 불러오는 중입니다.</p>
    </div>
  </article>

  <article
    v-else-if="section.model.testcaseErrorMessage && !section.model.testcaseItems.length"
    class="admin-testcase-card"
  >
    <div class="empty-state error-state compact-state">
      <p>{{ section.model.testcaseErrorMessage }}</p>
    </div>
  </article>

  <article
    v-else-if="!section.model.testcaseItems.length"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스를 추가하면 여기에서 바로 편집할 수 있습니다.</p>
    </div>
  </article>

  <article
    v-else-if="section.model.isLoadingSelectedTestcase"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스 본문을 불러오는 중입니다.</p>
    </div>
  </article>

  <article
    v-else-if="section.model.selectedTestcaseErrorMessage"
    class="admin-testcase-card"
  >
    <div class="empty-state error-state compact-state">
      <p>{{ section.model.selectedTestcaseErrorMessage }}</p>
    </div>
  </article>

  <article
    v-else-if="section.model.selectedTestcase"
    class="admin-testcase-card"
  >
    <AdminTestcaseMetadataHeader :section="metadataSection" />

    <div class="admin-testcases-grid">
      <AdminTextareaPanel
        :model-value="section.model.selectedTestcaseInputDraft"
        label="입력"
        placeholder="빈 입력도 허용됩니다."
        :disabled="Boolean(section.model.busySection)"
        @update:model-value="section.actions.updateSelectedTestcaseInputDraft"
      />

      <AdminTextareaPanel
        :model-value="section.model.selectedTestcaseOutputDraft"
        label="출력"
        placeholder="빈 출력도 허용됩니다."
        :disabled="Boolean(section.model.busySection)"
        @update:model-value="section.actions.updateSelectedTestcaseOutputDraft"
      />
    </div>

    <div class="admin-testcases-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!section.model.canSaveSelectedTestcase"
        @click="handleSaveSelectedTestcase"
      >
        {{ section.model.isSavingSelectedTestcase ? '저장 중...' : '저장' }}
      </button>
    </div>
  </article>

  <article
    v-else
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>왼쪽 요약에서 테스트케이스를 선택하면 본문을 보여줍니다.</p>
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

import AdminTextareaPanel from '@/components/adminShared/AdminTextareaPanel.vue'
import AdminTestcaseMetadataHeader from '@/components/admin-testcases/editor/AdminTestcaseMetadataHeader.vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

const metadataSection = computed(() => ({
  model: {
    selectedTestcase: section.value.model.selectedTestcase,
    canDeleteSelectedTestcase: section.value.model.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: section.value.model.isDeletingSelectedTestcase,
    formatCount: section.value.model.formatCount
  },
  actions: {
    deleteSelectedTestcase: section.value.actions.deleteSelectedTestcase
  }
}))

function handleSaveSelectedTestcase(){
  section.value.actions.saveSelectedTestcase()
}
</script>

<style scoped>
.admin-testcase-card {
  min-width: 0;
  overflow: hidden;
  padding: 1rem;
  border: 1px solid var(--admin-testcases-nested-border);
  border-radius: 20px;
  background: var(--admin-testcases-nested-surface);
  box-shadow: var(--admin-testcases-section-shadow);
}

.admin-testcases-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 1rem;
}

.admin-testcases-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1rem;
}

@media (max-width: 720px) {
  .admin-testcases-grid {
    grid-template-columns: 1fr;
  }

  .admin-testcases-actions {
    justify-content: stretch;
  }
}
</style>
