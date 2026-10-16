add_executable(tes_inti
    tes_utama.c
    tes_penyangga.c
    tes_json.c
    tes_pengodean.c
    tes_konfigurasi.c
    tes_alat.c)
target_link_libraries(tes_inti PRIVATE pustaka_inti)
add_test(NAME tes_inti COMMAND tes_inti)
