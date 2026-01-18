# Potenziali Bug e TODO Identificati

Questo file riassume i potenziali bug, TODO e aree di miglioramento identificate durante l'analisi del codice.

## Da Ricerca Testuale (`grep TODO|FIXME|BUG|HACK`)

1.  **Calcolo Potenza Impreciso (`main/power/power.c`)**
    *   Righe 104 e 118 contengono commenti `TODO: this better.` relativi all'aggiunta di un offset (`SUPRA_POWER_OFFSET`, `GAMMATURBO_POWER_OFFSET`) per stimare la potenza totale. Il metodo di calcolo potrebbe essere migliorato per maggiore precisione.

2.  **Workaround Bug Hardware TPS546 (`main/power/vcore.c`, `main/power/TPS546.c`)**
    *   `main/power/vcore.c` (Riga 34): Imposta `TPS546_INIT_VIN_UV_WARN_LIMIT` a 0 citando un "TI Bug in this register".
    *   `main/power/TPS546.c` (Riga 522): Commento `//deal with the UV_WARN_LIMIT bug`.
    *   Indica un workaround per un bug noto del componente TPS546. Da investigare se esistono soluzioni alternative o implicazioni del workaround attuale.

3.  **Mancata Scrittura `MFR_REVISION` (`main/power/TPS546.c`)**
    *   Riga 614: `/* TODO write new MFR_REVISION number to reflect these parameters */`.
    *   I parametri di configurazione modificati potrebbero non essere riflessi nel numero di revisione memorizzato nel TPS546, potenzialmente causando problemi di identificazione o compatibilità.

## Da Ricerca Semantica (`potential bug in code`)

1.  **Potenziale Perdita Notifiche Stratum (`main/tasks/stratum_task.c`, riga ~316)**
    *   Se la `stratum_queue` è piena (`count == QUEUE_SIZE`), la notifica più vecchia viene scartata per far posto alla nuova. Potrebbe causare mining brevemente obsoleto se le notifiche arrivano troppo velocemente o la coda è piccola.

2.  **Gestione Errore `realloc` Drastica (`components/stratum/stratum_api.c`, riga ~55)**
    *   Un fallimento di `realloc` nel buffer JSON causa un riavvio immediato del sistema (`esp_restart`). Potrebbe essere una gestione troppo aggressiva per un potenziale problema temporaneo di memoria.

3.  **Possibile Bug Logico Gestione Nonce Duplicati (`components/asic/bm1397.c`, righe ~419-448)**
    *   La logica in `BM1397_process_work` per identificare e scartare nonce duplicati o già visti (usando `prev_nonce`, `nonce_found`, `first_nonce`) potrebbe non essere completamente robusta in tutti gli scenari o edge case, specialmente considerando la possibile natura statica di `prev_nonce` (anche se non visibile nello snippet). **Questo sembra il candidato più probabile per un bug logico.**

4.  **Sicurezza Thread Code (`main/work_queue.c`, righe ~55, ~71)**
    *   Le funzioni di gestione della coda usano mutex (`pthread_mutex_lock`/`unlock`), suggerendo thread-safety. Un bug potrebbe esistere solo se ci fossero accessi diretti alla struttura della coda *senza* usare queste funzioni protette.

5.  **Gestione Discrepanza Numero Chip (`components/asic/common.c`, riga ~84)**
    *   Viene loggato solo un warning (`ESP_LOGW`) se il numero di chip rilevati non corrisponde a quello atteso. Potrebbe essere più robusto segnalare un errore (`ESP_LOGE`) o gestire attivamente questa condizione (es. blocco avvio mining) dato che indica un potenziale problema hardware/configurazione.

## Prossimi Passi Suggeriti

*   Investigare la logica di gestione dei nonce in `components/asic/bm1397.c` (Punto 3 della ricerca semantica).
*   Verificare se il workaround per il bug del TPS546 (Punto 2 della ricerca testuale) ha effetti collaterali o se esistono fix noti da TI.
*   Rivalutare la gestione dell'errore `realloc` (Punto 2 della ricerca semantica).
*   Migliorare la precisione del calcolo della potenza (Punto 1 della ricerca testuale). 