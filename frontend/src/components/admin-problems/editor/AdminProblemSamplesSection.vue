<template>
  <article class="admin-problem-editor-section admin-problem-editor-section--samples">
    <div class="panel-header admin-sample-section-header">
      <div>
        <p class="panel-kicker">samples</p>
        <h3>공개 예제</h3>
        <p class="admin-problem-section-copy">
          문제 상세 페이지에 노출되는 공개 예제를 행별로 관리합니다. 추가 시 빈 샘플이 생성되며, 삭제 버튼은 마지막 샘플에만 표시됩니다.
        </p>
      </div>

      <button
        type="button"
        class="ghost-button"
        :disabled="!section.model.canCreateSample"
        @click="handleCreateSample"
      >
        {{ section.model.isCreatingSample ? '추가 중...' : '샘플 추가' }}
      </button>
    </div>

    <div v-if="!section.model.sampleDrafts.length" class="empty-state compact-state">
      <p>등록된 공개 예제가 아직 없습니다.</p>
    </div>

    <div v-else class="admin-sample-list">
      <AdminProblemSampleCard
        v-for="sampleDraft in section.model.sampleDrafts"
        :key="sampleDraft.sample_order"
        :sample-draft="sampleDraft"
        :can-delete-last-sample="section.model.canDeleteLastSample"
        :is-deleting-last-sample="section.model.isDeletingLastSample"
        :format-count="section.model.formatCount"
        :is-saving-sample="section.model.isSavingSample"
        :can-save-sample="section.model.canSaveSample"
        :is-last-sample="section.model.isLastSample"
        @delete-last-sample="handleDeleteLastSample"
        @save-sample="handleSaveSample"
      />
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

import AdminProblemSampleCard from '@/components/admin-problems/editor/AdminProblemSampleCard.vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleCreateSample(){
  section.value.actions.createSample()
}

function handleSaveSample(sampleOrder){
  section.value.actions.saveSample(sampleOrder)
}

function handleDeleteLastSample(){
  section.value.actions.deleteLastSample()
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

.admin-sample-list {
  display: grid;
  gap: 1rem;
}

@media (max-width: 1100px) {
  .admin-sample-section-header {
    flex-direction: column;
  }
}
</style>
