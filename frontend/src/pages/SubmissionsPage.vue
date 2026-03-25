<template>
  <section class="page-grid single-column">
    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">submissions</p>
          <h3>제출 화면 뼈대</h3>
        </div>
        <StatusBadge label="Scaffolded" tone="success" />
      </div>

      <p class="lead-copy">
        제출 목록은 운영에 가장 도움이 되는 화면이라서, 로그인 다음으로 붙이기 좋은 페이지입니다.
      </p>

      <div class="todo-list">
        <div class="todo-item">
          <strong>목록 우선</strong>
          <p>채점 상태, 언어, 제출 시간, 사용자, 문제 번호 정도만 먼저 보여줘도 충분합니다.</p>
        </div>
        <div class="todo-item">
          <strong>상세는 별도 라우트</strong>
          <p>소스코드와 채점 결과는 목록과 분리하면 상태가 단순해집니다.</p>
        </div>
        <div class="todo-item">
          <strong>필터는 나중에 추가</strong>
          <p>상태 필터, 문제 필터, 사용자 필터는 기본 목록이 붙은 뒤에 넣는 편이 빠릅니다.</p>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
</script>
