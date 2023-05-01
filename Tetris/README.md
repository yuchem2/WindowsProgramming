# Tetris

윈도우즈 API 정복 1권, 한빛미디어 9장 실습 Tetris 게임 프로그래밍 구현
 + Date: 2022.11.03-2022.11.05


## 코드 분석

해당 프로그램은 테트리스 게임을 윈도우 환경에서 구현하는 것을 목표로 하고 있다. 

이 프로그램에서 가장 중요한 것은 실시간으로 블럭이 이동하는 것 처럼 보이게 하며 우리가 아는 테트리스의 모든 기능을 수행하는 것이다.

사용자가 게임판의 크기를 조정할 수 있고, 이에 따라 조정할 수 있는 최대 크기를 2차원 정적 배열을 이용해 구현하였다.

또한 각 블록에 대한 정보를 3차원 정적 배열로 구현해 각 블록을 손쉽게 게임판 배열에 그 블록을 업데이트 할 수 있도록 구현하였다. 

각 메시지는 윈도우즈 API 프로그래밍의 방식에 따라, 메시지 마다 따로 관리하며 메시지 스위치 구문에서 반복을 최소한으로 하게 구현하였다. 

## 실행화면 예시
<p>
<img src="https://user-images.githubusercontent.com/101711808/235471531-5084997c-2706-42ad-a01a-16beef82765e.png" width="330" height="400">
<img src="https://user-images.githubusercontent.com/101711808/235471822-94ecdebe-fd0d-49d3-ac90-9b6d1d5b13d5.png" width="330" height="400">
<img src="https://user-images.githubusercontent.com/101711808/235472515-dfc2e59f-f704-4126-b7ae-6f04729f7de2.png" width="330" height="400">
</p>
