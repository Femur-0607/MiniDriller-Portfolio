# Notion 문서 동기화 자동화

이 저장소는 Notion 문서를 GitHub 문서로 자동 동기화하기 위한 워크플로를 포함합니다.

## 추가된 파일

- `.github/workflows/sync-notion-docs.yml`: GitHub Actions 자동 실행 설정
- `scripts/sync-notion-docs.mjs`: Notion API에서 문서를 읽어 Markdown으로 변환하는 스크립트
- `Docs/notion-sync.config.json`: 동기화할 Notion 페이지와 데이터베이스 목록

## 동작 방식

GitHub Actions가 Notion API를 호출해서 지정된 페이지와 개발 일지 데이터베이스를 읽고, 결과물을 `Docs/Notion/` 아래에 생성합니다. 기존 `Docs/` 문서는 그대로 두고, 자동 생성 문서는 별도 폴더에만 기록합니다.

Notion 내부 파일로 등록된 이미지, 영상, 첨부 파일은 동기화 중 `Docs/Notion/assets/`에 저장하고, Markdown에서는 저장된 로컬 파일을 참조합니다. 외부 URL로 등록된 링크는 원본 URL을 유지합니다.

기본 실행 주기는 매주 월요일 03:00 KST입니다. GitHub Actions 화면에서 수동 실행도 가능합니다.

## 최초 설정

1. Notion에서 내부 통합을 만들고 integration token을 발급합니다.
2. 동기화할 Notion 페이지와 개발 일지 데이터베이스를 해당 integration에 공유합니다.
3. GitHub 저장소에서 `Settings > Secrets and variables > Actions > New repository secret`으로 이동합니다.
4. 이름은 `NOTION_TOKEN`, 값은 Notion integration token으로 저장합니다.
5. `Actions > Sync Notion Docs > Run workflow`로 수동 실행합니다.

## 결과물 위치

첫 실행 후 다음 파일들이 생성됩니다.

- `Docs/Notion/README.md`
- `Docs/Notion/index.md`
- `Docs/Notion/project-main.md`
- `Docs/Notion/checklist.md`
- `Docs/Notion/backlog.md`
- `Docs/Notion/troubleshooting.md`
- `Docs/Notion/paperzd-guide.md`
- `Docs/Notion/dev-logs/README.md`
- `Docs/Notion/dev-logs/*.md`
- `Docs/Notion/assets/*`

## 주의사항

`Docs/Notion/` 아래 파일은 자동 생성물입니다. 내용을 직접 수정하면 다음 동기화 때 덮어써질 수 있습니다.

Notion에서 외부 URL로 삽입한 이미지나 영상은 GitHub로 복사하지 않고 원본 링크를 유지합니다. GitHub에 고정 보관하고 싶은 자료는 Notion에 파일로 업로드한 뒤 동기화하는 편이 안정적입니다.

문서 목록을 바꾸려면 `Docs/notion-sync.config.json`의 `pages` 또는 `databases` 항목을 수정합니다.
