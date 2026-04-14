<template>
  <article
    v-if="isLoadingTestcases"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스 요약을 불러오는 중입니다.</p>
    </div>
  </article>

  <article
    v-else-if="testcaseErrorMessage && !testcaseItems.length"
    class="admin-testcase-card"
  >
    <div class="empty-state error-state compact-state">
      <p>{{ testcaseErrorMessage }}</p>
    </div>
  </article>

  <article
    v-else-if="!testcaseItems.length"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스를 추가하면 여기에서 바로 편집할 수 있습니다.</p>
    </div>
  </article>

  <article
    v-else-if="isLoadingSelectedTestcase"
    class="admin-testcase-card"
  >
    <div class="empty-state compact-state">
      <p>테스트케이스 본문을 불러오는 중입니다.</p>
    </div>
  </article>

  <article
    v-else-if="selectedTestcaseErrorMessage"
    class="admin-testcase-card"
  >
    <div class="empty-state error-state compact-state">
      <p>{{ selectedTestcaseErrorMessage }}</p>
    </div>
  </article>

  <article
    v-else-if="selectedTestcase"
    class="admin-testcase-card"
  >
    <AdminTestcaseMetadataHeader
      :selected-testcase="selectedTestcase"
      :can-delete-selected-testcase="canDeleteSelectedTestcase"
      :is-deleting-selected-testcase="isDeletingSelectedTestcase"
      :format-count="formatCount"
      @delete-selected-testcase="$emit('delete-selected-testcase')"
    />

    <div class="admin-testcases-grid">
      <AdminTestcaseInputPanel
        :selected-testcase-input-draft="selectedTestcaseInputDraft"
        :busy-section="busySection"
        @update:selected-testcase-input-draft="$emit('update:selectedTestcaseInputDraft', $event)"
      />

      <AdminTestcaseOutputPanel
        :selected-testcase-output-draft="selectedTestcaseOutputDraft"
        :busy-section="busySection"
        @update:selected-testcase-output-draft="$emit('update:selectedTestcaseOutputDraft', $event)"
      />
    </div>

    <div class="admin-testcases-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!canSaveSelectedTestcase"
        @click="$emit('save-selected-testcase')"
      >
        {{ isSavingSelectedTestcase ? '저장 중...' : '저장' }}
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
import AdminTestcaseInputPanel from '@/components/admin-testcases/editor/AdminTestcaseInputPanel.vue'
import AdminTestcaseMetadataHeader from '@/components/admin-testcases/editor/AdminTestcaseMetadataHeader.vue'
import AdminTestcaseOutputPanel from '@/components/admin-testcases/editor/AdminTestcaseOutputPanel.vue'

defineProps({
  isLoadingTestcases: {
    type: Boolean,
    required: true
  },
  testcaseErrorMessage: {
    type: String,
    required: true
  },
  testcaseItems: {
    type: Array,
    required: true
  },
  isLoadingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  selectedTestcaseErrorMessage: {
    type: String,
    required: true
  },
  selectedTestcase: {
    type: Object,
    default: null
  },
  canDeleteSelectedTestcase: {
    type: Boolean,
    required: true
  },
  isDeletingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  busySection: {
    type: String,
    default: ''
  },
  selectedTestcaseInputDraft: {
    type: String,
    default: ''
  },
  selectedTestcaseOutputDraft: {
    type: String,
    default: ''
  },
  canSaveSelectedTestcase: {
    type: Boolean,
    required: true
  },
  isSavingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  }
})

defineEmits([
  'delete-selected-testcase',
  'update:selectedTestcaseInputDraft',
  'update:selectedTestcaseOutputDraft',
  'save-selected-testcase'
])
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
