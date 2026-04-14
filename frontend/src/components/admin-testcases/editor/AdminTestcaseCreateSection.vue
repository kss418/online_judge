<template>
  <article class="admin-testcases-section admin-testcases-section--append">
    <div class="panel-header">
      <div>
        <p class="panel-kicker">append</p>
        <h3>테스트케이스 추가</h3>
      </div>
    </div>

    <div class="admin-testcases-grid">
      <label class="field-block">
        <span class="field-label">입력</span>
        <textarea
          :value="section.model.newTestcaseInput"
          class="admin-testcases-textarea"
          spellcheck="false"
          :disabled="Boolean(section.model.busySection)"
          placeholder="빈 입력도 허용됩니다."
          @input="handleNewTestcaseInput"
        />
      </label>

      <label class="field-block">
        <span class="field-label">출력</span>
        <textarea
          :value="section.model.newTestcaseOutput"
          class="admin-testcases-textarea"
          spellcheck="false"
          :disabled="Boolean(section.model.busySection)"
          placeholder="빈 출력도 허용됩니다."
          @input="handleNewTestcaseOutput"
        />
      </label>
    </div>

    <div class="admin-testcases-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!section.model.canCreateTestcase"
        @click="handleCreateTestcase"
      >
        {{ section.model.isCreatingTestcase ? '추가 중...' : '테스트케이스 추가' }}
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

function handleNewTestcaseInput(event){
  section.value.actions.updateNewTestcaseInput(event.target.value)
}

function handleNewTestcaseOutput(event){
  section.value.actions.updateNewTestcaseOutput(event.target.value)
}

function handleCreateTestcase(){
  section.value.actions.createTestcase()
}
</script>

<style scoped>
.admin-testcases-section {
  display: grid;
  gap: 1rem;
  padding: 1.2rem;
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  background: var(--admin-testcases-section-surface);
  box-shadow: var(--admin-testcases-section-shadow);
}

.admin-testcases-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 1rem;
}

.admin-testcases-textarea {
  width: 100%;
  min-height: 12rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-testcases-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  resize: vertical;
  line-height: 1.6;
}

.admin-testcases-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
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
