이 프로젝트는 PC에 Python/.NET/컴파일러를 설치하지 않고,
GitHub의 Windows 빌드 머신에서 ScreenComment.exe를 만들어 다운로드하는 용도입니다.

사용:
1. github.com에서 새 Repository를 만듭니다.
2. 이 ZIP의 파일들을 업로드합니다.
3. Actions 탭 → "Build ScreenComment.exe" → Run workflow.
4. 작업이 끝나면 Artifacts의 ScreenComment를 다운로드합니다.
5. 압축을 풀면 ScreenComment.exe 하나가 있습니다.

GitHub Actions의 Windows runner에는 Windows 빌드 도구가 포함되어 있습니다.
