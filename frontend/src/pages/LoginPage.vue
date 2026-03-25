<template>
  <section class="page-grid single-column">
    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">login</p>
          <h3>인증 화면 뼈대</h3>
        </div>
        <StatusBadge label="Ready" tone="warning" />
      </div>

      <p class="lead-copy">
        인증은 이후 모든 화면의 진입점이기 때문에, 실제 개발을 시작할 때 가장 먼저 구현하는 걸 추천합니다.
      </p>

      <div class="todo-list">
        <div class="todo-item">
          <strong>로그인 폼</strong>
          <p>아이디와 비밀번호만 먼저 받고, 성공 시 토큰 저장까지 단일 흐름으로 붙입니다.</p>
        </div>
        <div class="todo-item">
          <strong>회원가입 분리</strong>
          <p>같은 페이지 탭보다 별도 라우트나 섹션으로 나누는 편이 AI에게 지시하기도 쉽습니다.</p>
        </div>
        <div class="todo-item">
          <strong>권한 확인</strong>
          <p>현재 사용자 조회 API를 붙여 관리자 전용 메뉴를 나누면 운영 화면이 깔끔해집니다.</p>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
</script>
