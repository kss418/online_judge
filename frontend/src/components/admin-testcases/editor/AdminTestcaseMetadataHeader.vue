<template>
  <div class="admin-testcase-card-header">
    <div>
      <p class="panel-kicker">testcase {{ section.model.formatCount(section.model.selectedTestcase.testcase_order) }}</p>
      <h3>테스트케이스 {{ section.model.formatCount(section.model.selectedTestcase.testcase_order) }}</h3>
    </div>

    <button
      type="button"
      class="ghost-button admin-testcase-delete-button"
      :disabled="!section.model.canDeleteSelectedTestcase"
      @click="handleDeleteSelectedTestcase"
    >
      {{ section.model.isDeletingSelectedTestcase ? '삭제 중...' : '테스트케이스 삭제' }}
    </button>
  </div>
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

function handleDeleteSelectedTestcase(){
  section.value.actions.deleteSelectedTestcase()
}
</script>

<style scoped>
.admin-testcase-card-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
  margin-bottom: 1rem;
}

.admin-testcase-delete-button {
  flex: 0 0 auto;
}

@media (max-width: 720px) {
  .admin-testcase-card-header {
    align-items: start;
    flex-direction: column;
  }
}
</style>
