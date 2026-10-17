# MyOS — учебная микро-ОС для x86

Микро-ОС для архитектуры i686, написанная с нуля на C и NASM.
Загружается через QEMU с multiboot-заголовком (`-kernel`).
Цель — пошаговое изучение устройства ОС: от загрузки до многозадачности
и системных вызовов.

---

## Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Архитектура](#архитектура)
3. [Что уже работает](#что-уже-работает)
4. [Структура проекта](#структура-проекта)
5. [Ключевые подсистемы](#ключевые-подсистемы)
   - [Загрузка](#загрузка)
   - [Память](#память)
   - [Прерывания](#прерывания)
   - [Таймер](#таймер)
   - [Клавиатура](#клавиатура)
   - [Многозадачность](#многозадачность)
   - [Системные вызовы](#системные-вызовы)
   - [Файловая система](#файловая-система)
   - [Shell](#shell)
6. [Команды shell](#команды-shell)
7. [Известные ограничения](#известные-ограничения)
8. [Куда развивать](#куда-развивать)

---

## Быстрый старт

### Требования

- `i686-linux-gnu-gcc` — кросс-компилятор
- `nasm` — ассемблер
- `qemu-system-i386` — эмулятор

Установка (Ubuntu/Debian):

```bash
sudo apt install gcc-i686-linux-gnu nasm qemu-system-x86

Сборка и запуск
bash

make clean && make && make run

make run запускает QEMU с последовательным портом, выведенным в терминал
(-serial mon:stdio). Это значит, что:

    serial-вывод (отладочные сообщения ядра) идёт в терминал

    VGA-вывод (shell) идёт в окно QEMU

Можно переключаться между окнами: shell — в окне QEMU,
serial — в терминале, откуда запущен make run.
Выход из QEMU

Ctrl+A, затем X — в терминале с serial.
Архитектура

    Архитектура: x86 (i686), 32-bit protected mode

    Загрузка: Multiboot (QEMU -kernel)

    Язык: C (freestanding) + NASM

    Память: flat model, без страничной адресации

    Привилегии: все процессы в ring 0 (ядро)

    Планировщик: Round Robin с квантами

    Файловая система: в памяти (RAM FS)

    Драйверы: PIT (таймер), PS/2 клавиатура (IRQ1), VGA текст 80×25

    Системные вызовы: int 0x80

Что уже работает
Компонент	Статус
Multiboot-загрузка (boot.asm)	✅
VGA-текст 80×25, цвета	✅
Serial COM1 (отладка)	✅
kprintf с %d, %x, %s, %c, %%	✅
Heap kmalloc/kfree (пул 1 МБ, coalesce)	✅
IDT, PIC remap	✅
Таймер PIT 100 Гц	✅
Драйвер клавиатуры через IRQ1 (shift, caps, стрелки)	✅
Планировщик Round Robin	✅
Переключение контекста (switch.asm)	✅
Процессы: idle, dummy, syscall_test	✅
ps, run, kill, sleep	✅
Системные вызовы int 0x80 (6 вызовов)	✅
Файловая система в памяти	✅
Shell: история, стрелки, ;-разделитель	✅
uptime_ms, sleep_ms	✅
Структура проекта
text

microkernel-project/
├── Makefile
├── linker.ld
├── README.md
├── src/
│   ├── boot/
│   │   └── boot.asm          # Multiboot-заголовок, _start
│   └── kernel/
│       ├── kernel.c          # kernel_main, тестовые процессы
│       ├── screen.c          # VGA текст, kprintf
│       ├── serial.c          # COM1, print_serial
│       ├── string.c          # strlen, strcmp, memcpy, ...
│       ├── kheap.c           # kmalloc/kfree, пул 1 МБ
│       ├── memory.c          # обёртка над kheap
│       ├── interrupts.c      # IDT, PIC, irq_handler
│       ├── interrupts.asm    # ISR/IRQ стабы, syscall_stub
│       ├── keyboard.c        # драйвер PS/2 (IRQ1)
│       ├── process.c         # планировщик, процессы
│       ├── switch.asm        # switch_context
│       ├── syscall.c         # обработчик int 0x80
│       ├── fs.c              # файловая система в памяти
│       └── shell.c           # команды, история, ввод
└── include/
    ├── screen.h
    ├── serial.h
    ├── string.h
    ├── kheap.h
    ├── memory.h
    ├── interrupts.h
    ├── keyboard.h
    ├── process.h
    ├── syscall.h
    ├── fs.h
    └── shell.h

Ключевые подсистемы
Загрузка

boot.asm содержит Multiboot-заголовок и точку входа _start:

    Сохраняет eax (magic) и ebx (адрес multiboot info) в переменные

    Настраивает стек

    Вызывает kernel_main

kernel_main инициализирует подсистемы в порядке:
text

serial → screen → kheap → interrupts → syscall → keyboard
       → process → fs → shell

Затем включает прерывания (sti) и уходит в hlt-цикл.
Всё дальнейшее — через прерывания (IRQ0, IRQ1, int 0x80).
Память

Heap: пул 1 МБ (heap_pool[1<<20]), kmalloc/kfree с:

    заголовком Block (size, free, next, prev)

    выравниванием 8 байт

    разбиением блока при выделении

    coalesce (слияние соседних свободных) при освобождении

API:
c

void* kmalloc(size_t size);
void  kfree(void* ptr);
size_t kheap_used(void);
size_t kheap_free(void);

Стек процесса: KERNEL_STACK_SIZE = 4096 байт на процесс
(выделяется из того же heap).

Виртуальной памяти нет — flat model, все адреса физические.
Прерывания

IDT — 256 записей, заполнены для:

    isr0, isr1, isr6, isr8, isr13, isr14 (исключения)

    irq0 (таймер, вектор 32), irq1 (клавиатура, вектор 33)

    syscall_stub (вектор 0x80)

PIC remap: IRQ0-7 → 32-39, IRQ8-15 → 40-47.

Обработчик irq_handler:
c

void irq_handler(struct regs* r) {
    int irq = r->int_no - 32;

    // EOI СРАЗУ — до scheduler_tick, чтобы PIC не блокировался
    if (irq >= 8) outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);

    if (irq == 0) scheduler_tick();      // таймер
    if (irq == 1) keyboard_handler();    // клавиатура
}

Важно: EOI отправляется до обработки. Иначе PIC не даст
следующее прерывание, пока не получит EOI.
Таймер

PIT (8253/8254), канал 0, режим 3 (square wave), частота 100 Гц.
c

static void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

Каждый тик (10 мс):

    scheduler_tick() — увеличивает timer_ticks, проверяет time_used

    process_wakeup() — уменьшает счётчики сна в blocked_queue

    Если у текущего процесса time_used >= time_slice — переключение

API:
c

uint32_t timer_get_ticks(void);   // сколько тиков прошло
uint32_t uptime_ms(void);          // сколько мс с загрузки
void process_sleep_ms(uint32_t ms);// сон в миллисекундах

Клавиатура

Драйвер PS/2, IRQ1.

keyboard_handler():

    Читает scancode из порта 0x60

    Обрабатывает 0xE0 — префикс расширенных клавиш

    Для нажатия (scancode < 128):

        Shift (0x2A, 0x36) — включить shift

        Caps Lock (0x3A) — переключить

        Стрелки (0x48, 0x50, 0x4B, 0x4D) — передать в shell как 0x80-0x83

        Обычные клавиши — shell_process_input(c)

    Для отпускания — сбросить shift

EOI отправляется в irq_handler (не в keyboard_handler).

Карты: keymap[128] (без shift), keymap_shift[128] (с shift).
Caps Lock инвертирует регистр букв.
Многозадачность

Round Robin с квантами (time_slice).
c

typedef struct Process {
    int pid;
    char name[32];
    ProcessState state;   // NEW, READY, RUNNING, BLOCKED, TERMINATED
    ProcessContext context;
    struct Process* next;
    int priority;
    uint32_t time_slice;
    uint32_t time_used;
    uint32_t stack_base;
    uint32_t stack_size;
    int exit_code;
    struct Process* parent;
} Process;

Очереди:

    ready_queue — процессы, готовые к выполнению

    blocked_queue — спящие (ждут wakeup_tick)

    process_table[MAX_PROCESSES=64] — все процессы по pid

Переключение контекста (switch_context):
asm

; void switch_context(uint32_t* old_esp, uint32_t new_esp)
mov eax, [esp+4]      ; old_esp_ptr
mov edx, [esp+8]      ; new_esp
push ebp
push ebx
push esi
push edi
mov [eax], esp        ; сохранить старый esp
mov esp, edx          ; загрузить новый
pop edi
pop esi
pop ebx
pop ebp
ret                   ; прыжок на entry (или продолжение)

prepare_stack кладёт на стек нового процесса 5 значений:
edi=0, esi=0, ebx=0, ebp=0, eip=entry.
После ret процесс начинает выполнение с entry.

Создание процесса:
c

Process* p = process_create_kernel("name", entry_func);
enqueue_ready(p);   // добавить в очередь

Завершение:
c

process_exit(code);       // изнутри процесса
process_kill(pid);        // снаружи

Сон:
c

process_sleep(ticks);     // в тиках
process_sleep_ms(ms);     // в миллисекундах

Процесс уходит в blocked_queue с time_used = ticks.
Каждый тик process_wakeup() уменьшает счётчик.
Когда time_used == 0 — процесс возвращается в ready_queue.
Системные вызовы

int 0x80 — программное прерывание для обращения к ядру.

Соглашение:

    eax — номер вызова

    ebx, ecx, edx — аргументы

    результат — в eax

Обработчик syscall_stub (NASM):

    Сохраняет регистры ebp, edi, esi, edx, ecx, ebx, eax

    Передаёт esp как указатель на массив [eax, ebx, ecx, edx, ...]

    Вызывает syscall_handler(uint32_t* regs) из C

    Восстанавливает регистры, sti, iret

Таблица вызовов:
№	Имя	Аргументы	Что делает
1	sys_write	ebx=строка	печатает строку
2	sys_exit	ebx=код	завершает процесс
3	sys_sleep	ebx=ms	спит
4	sys_getpid	—	возвращает pid
5	sys_uptime	—	возвращает uptime_ms
6	sys_putchar	ebx=символ	печатает символ

Пример вызова из C:
c

const char* msg = "hello";
asm volatile (
    "int $0x80\n"
    :
    : "a"(1), "b"(msg)   // eax=1, ebx=msg
    : "memory"
);

Файловая система

В памяти (RAM FS), максимум 100 записей.

Каждая запись — FileEntry:
c

typedef struct {
    char name[MAX_NAME_LENGTH];   // имя
    char* data;                    // данные (kmalloc)
    size_t size;                   // размер
    size_t capacity;
    int is_directory;
    int parent_index;              // индекс родителя
    int in_use;
    Permissions perms;             // rwx-биты
    int owner_id, group_id;
} FileEntry;

Корень — files[0], имя /, parent_index = -1.

Пути: абсолютные (/home/user) и относительные (home/user).
.. — переход к родителю.

API:
c

void fs_init(void);
int  fs_create_file(const char* name, int parent);
int  fs_create_dir(const char* name, int parent);
int  fs_write_file(const char* name, const char* data, int parent);
const char* fs_read_file(const char* name, int parent);
int  fs_delete_entry(const char* name, int parent);
int  fs_change_dir(const char* path);
int  fs_mkdir_p(const char* path);
int  fs_rename_entry(const char* old, const char* new, int parent);
int  fs_copy_file(const char* src, const char* dst, int parent);
int  fs_find_files(const char* path, const char* pattern);
int  fs_find_entry(const char* name, int dir);
void fs_list_directory(int dir, char* buf, size_t size);
void fs_tree(const char* path, int show_hidden);
FileInfo fs_get_info(const char* name, int parent);
int  fs_set_permissions(const char* name, int parent, Permissions p);

Права: 9 бит (owner/group/others × rwx). chmod принимает 3-цифровое
восьмеричное число (644, 755).
Shell

Работает в контексте процесса idle (главный цикл kernel_main).

Возможности:

    История команд (20 записей, стрелки Up/Down)

    Backspace, Caps Lock, Shift

    ; — разделитель нескольких команд в одной строке

    Ввод через IRQ1 → shell_process_input(c)

Ввод:

    c >= 32 && c <= 126 — обычные символы

    \n (Enter) — выполнить команду

    \b (Backspace) — удалить символ

    0x80 — стрелка Up

    0x81 — стрелка Down

Выполнение:

    shell_execute_command(cmd) — разбирает строку

    Каждая команда проверяется через strcmp / starts_with

    Результат — kprintf в VGA + print_serial в отладку

Команды shell
Общие
Команда	Описание
help, ?	показать справку
clear, cls	очистить экран
history	история команд
uptime	время с загрузки
reboot	перезагрузка (заглушка)
Память
Команда	Описание
memory	статистика heap
Процессы
Команда	Описание
ps	список процессов
run <name>	создать процесс с именем <name>
kill <pid>	убить процесс по pid
sleep <ms>	усыпить shell на N миллисекунд
Файловая система
Команда	Описание
ls	содержимое текущей директории
pwd	текущий путь
cd <dir>	перейти в директорию
cd ..	на уровень выше
mkdir <dir>	создать директорию (с путями)
touch <file>	создать пустой файл
cat <file>	показать содержимое
rm <file>	удалить файл/директорию
echo text > file	записать в файл
stat <file>	информация о файле
chmod <mode> <file>	изменить права (644, 755)
find <path> <pat>	найти файлы по подстроке
tree	дерево директорий
tree -a	дерево с hidden-файлами
tree <path>	дерево указанной директории
mv <old> <new>	переименовать
cp <src> <dst>	скопировать
Примеры
text

mkdir /home/user/docs
touch /home/user/docs/readme.txt
echo hello world > /home/user/docs/readme.txt
cat /home/user/docs/readme.txt
find /home txt
tree /home
cp /home/user/docs/readme.txt /home/user/docs/copy.txt
chmod 755 /home/user/docs/readme.txt

Известные ограничения

    Нет виртуальной памяти — все адреса физические, процессы видят
    всё адресное пространство.

    Все процессы в ring 0 — нет изоляции, пользовательские программы
    могут делать всё, что и ядро.

    Нет fork/exec — процессы создаются только через run <name>
    из shell (entry-функция задана в ядре).

    ФС в памяти — при перезагрузке всё теряется.

    print_serial не атомарный — при частом выводе из нескольких
    процессов строки перемешиваются.

    idle в ready_queue — обязательно, иначе клавиатура не работает
    при 2+ процессах (idle должен получать квант).

    PIT дрейфует — uptime_ms может отличаться от реального времени
    на секунды в час.

    Нет сети — драйвер сетевой карты не реализован.

    Нет ATA/IDE — чтение/запись на реальный диск не поддерживается.

    Один CPU — планировщик не рассчитан на SMP.


Стиль проекта

    Freestanding C — без libc, только свои string.c, kheap.c

    NASM — для asm-стабов (ISR, IRQ, switch, syscall)

    Multiboot — загрузка через QEMU -kernel

    Отладка через serial — все важные события дублируются в COM1

    Без внешних зависимостей — только gcc-i686, nasm, qemu

Лицензия

Учебный проект. Используйте как хотите.
