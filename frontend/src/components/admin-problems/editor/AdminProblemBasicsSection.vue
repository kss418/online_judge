<template>
  <div class="admin-problem-basics-stack">
    <article class="admin-problem-editor-section admin-problem-editor-section--title">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">title</p>
          <h3>제목</h3>
        </div>
      </div>

      <div class="admin-problem-form-grid single-column-grid">
        <input
          :value="section.model.titleDraft"
          class="field-input"
          type="text"
          maxlength="120"
          placeholder="문제 제목"
          @input="handleTitleInput"
        />
      </div>

      <div class="admin-problem-section-actions">
        <button
          type="button"
          class="primary-button"
          :disabled="!section.model.canSaveTitle"
          @click="handleSaveTitle"
        >
          {{ section.model.isSavingTitle ? '저장 중...' : '저장' }}
        </button>
      </div>
    </article>

    <article class="admin-problem-editor-section admin-problem-editor-section--limits">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">limits</p>
          <h3>실행 제한</h3>
        </div>
      </div>

      <div class="admin-problem-form-grid">
        <label class="field-block">
          <span class="field-label">시간 제한(ms)</span>
          <input
            :value="section.model.timeLimitDraft"
            class="field-input"
            type="number"
            min="1"
            inputmode="numeric"
            placeholder="1000"
            @input="handleTimeLimitInput"
          />
        </label>

        <label class="field-block">
          <span class="field-label">메모리 제한(MB)</span>
          <input
            :value="section.model.memoryLimitDraft"
            class="field-input"
            type="number"
            min="1"
            inputmode="numeric"
            placeholder="256"
            @input="handleMemoryLimitInput"
          />
        </label>
      </div>

      <div class="admin-problem-section-actions">
        <button
          type="button"
          class="primary-button"
          :disabled="!section.model.canSaveLimits"
          @click="handleSaveLimits"
        >
          {{ section.model.isSavingLimits ? '저장 중...' : '저장' }}
        </button>
      </div>
    </article>
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

function handleTitleInput(event){
  section.value.actions.updateTitleDraft(event.target.value)
}

function handleTimeLimitInput(event){
  section.value.actions.updateTimeLimitDraft(event.target.value)
}

function handleMemoryLimitInput(event){
  section.value.actions.updateMemoryLimitDraft(event.target.value)
}

function handleSaveTitle(){
  section.value.actions.saveTitle()
}

function handleSaveLimits(){
  section.value.actions.saveLimits()
}
</script>

<style scoped>
.admin-problem-basics-stack {
  display: grid;
  gap: 1rem;
}

.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-problem-form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0.9rem;
}

.single-column-grid {
  grid-template-columns: minmax(0, 1fr);
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

@media (max-width: 720px) {
  .admin-problem-form-grid {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-problem-section-actions {
    justify-content: stretch;
  }
}
</style>
