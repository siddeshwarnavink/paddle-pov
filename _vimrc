compiler msvc
set makeprg=MSBuild.exe\ /nologo\ /verbosity:minimal\ /p:Configuration=Debug\ Paddle.sln

nnoremap <F6> :! .\x64\Debug\Paddle.exe<CR>
