<template>
  <div class="admin-testcase-card-header">
    <div>
      <p class="panel-kicker">testcase {{ formatCount(selectedTestcase.testcase_order) }}</p>
      <h3>테스트케이스 {{ formatCount(selectedTestcase.testcase_order) }}</h3>
    </div>

    <button
      type="button"
      class="ghost-button admin-testcase-delete-button"
      :disabled="!canDeleteSelectedTestcase"
      @click="$emit('delete-selected-testcase')"
    >
      {{ isDeletingSelectedTestcase ? '삭제 중...' : '테스트케이스 삭제' }}
    </button>
  </div>
</template>

<script setup>
defineProps({
  selectedTestcase: {
    type: Object,
    required: true
  },
  canDeleteSelectedTestcase: {
    type: Boolean,
    required: true
  },
  isDeletingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  }
})

defineEmits(['delete-selected-testcase'])
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
