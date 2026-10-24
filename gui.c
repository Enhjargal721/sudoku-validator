#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include "sudoku_core.h"

/* ---------- Global widgets ---------- */

static GtkWidget *status_label;
static GtkWidget *result_label;
static GtkWidget *sudoku_grid;
static GtkWidget *page_stack;
static GtkWidget *thread_status_labels[TOTAL_THREADS];
static GtkWidget *check_button_widget;
static GtkWidget *clear_button_widget;

static GtkCssProvider *cell_css;

static gboolean validation_running = FALSE;
static int last_font_px = 0;
static int pending_font_px = 0;

typedef struct {
    int validation_result;
} ValidationTask;

/* ---------- CSS ---------- */

static void setup_css(void)
{
    GtkCssProvider *css = gtk_css_provider_new();

    const char *style =
        "window {"
        " background-color: #F3F6FC;"
        " font-family: 'Segoe UI', sans-serif;"
        "}"

        "#welcome-title {"
        " font-size: 27px;"
        " font-weight: bold;"
        " color: #15345B;"
        "}"

        "#welcome-subtitle {"
        " font-size: 16px;"
        " color: #243B57;"
        "}"

        ".section-heading {"
        " font-size: 21px;"
        " font-weight: bold;"
        " color: #15345B;"
        "}"

        ".rule-card {"
        " background-color: #FFFFFF;"
        " border: 1px solid #B9CBE0;"
        " border-radius: 8px;"
        " padding: 10px;"
        "}"

        ".rule-number {"
        " font-size: 22px;"
        " font-weight: bold;"
        " color: #174EA6;"
        "}"

        ".rule-text {"
        " font-size: 16px;"
        " font-weight: bold;"
        " color: #183153;"
        "}"

        ".rule-description {"
        " font-size: 14px;"
        " color: #27384D;"
        "}"

        ".notice {"
        " background-color: #E5EDF8;"
        " color: #183153;"
        " border: 1px solid #B9CBE0;"
        " border-radius: 8px;"
        " padding: 10px;"
        " font-size: 14px;"
        "}"

        ".primary-button {"
        " background-image: none;"
        " background-color: #E2E8F0;"
        " color: #183153;"
        " border: 2px solid #AABBD0;"
        " border-radius: 8px;"
        " padding: 10px 20px;"
        " font-size: 16px;"
        " font-weight: bold;"
        " box-shadow: none;"
        "}"

        ".primary-button label {"
        " color: #183153;"
        " font-weight: bold;"
        "}"

        ".primary-button:hover {"
        " background-image: none;"
        " background-color: #CBD8E8;"
        "}"

        ".secondary-button {"
        " background-image: none;"
        " background-color: #E2E8F0;"
        " color: #1E293B;"
        " border: 1px solid #AABBD0;"
        " border-radius: 7px;"
        " padding: 7px 12px;"
        " font-size: 14px;"
        " font-weight: bold;"
        " box-shadow: none;"
        "}"

        ".secondary-button label {"
        " color: #1E293B;"
        " font-weight: bold;"
        "}"

        ".secondary-button:hover {"
        " background-image: none;"
        " background-color: #CBD5E1;"
        "}"

        ".success {"
        " background-color: #D1FAE5;"
        " color: #065F46;"
        " padding: 7px;"
        " font-size: 14px;"
        " font-weight: bold;"
        " border-radius: 6px;"
        "}"

        ".failure {"
        " background-color: #FEE2E2;"
        " color: #991B1B;"
        " padding: 7px;"
        " font-size: 14px;"
        " font-weight: bold;"
        " border-radius: 6px;"
        "}"

        ".neutral {"
        " color: #24364B;"
        " padding: 4px;"
        " font-size: 13px;"
        "}"

        ".thread-label {"
        " background-color: #FFFFFF;"
        " color: #24364B;"
        " border: 1px solid #D1DCE9;"
        " border-radius: 5px;"
        " padding: 5px;"
        " font-size: 12px;"
        "}"

        "entry.sudoku-cell {"
        " font-weight: bold;"
        " border: 1px solid #8FA4BE;"
        " border-radius: 3px;"
        " padding: 0;"
        " min-width: 0;"
        " min-height: 0;"
        "}"

        ".block-a {"
        " background-color: #DCEBFA;"
        " color: #183153;"
        "}"

        ".block-b {"
        " background-color: #E4F4E7;"
        " color: #183153;"
        "}"

        ".block-c {"
        " background-color: #FFF0D5;"
        " color: #183153;"
        "}"

        "entry.sudoku-cell:focus {"
        " background-color: #FFF3A6;"
        " border: 2px solid #174EA6;"
        "}";

    gtk_css_provider_load_from_data(css, style, -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(css);

    cell_css = gtk_css_provider_new();

    gtk_css_provider_load_from_data(
        cell_css,
        "entry.sudoku-cell { font-size: 20px; }",
        -1,
        NULL
    );

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(cell_css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1
    );
}

/* ---------- UI helpers ---------- */

static GtkWidget *make_label(
    const char *text,
    const char *css_class
)
{
    GtkWidget *label = gtk_label_new(text);

    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);

    if (css_class != NULL) {
        gtk_style_context_add_class(
            gtk_widget_get_style_context(label),
            css_class
        );
    }

    return label;
}

static GtkWidget *make_button(
    const char *text,
    const char *css_class
)
{
    GtkWidget *button = gtk_button_new_with_label(text);

    gtk_style_context_add_class(
        gtk_widget_get_style_context(button),
        css_class
    );

    return button;
}

static void set_status(
    const char *message,
    const char *css_class
)
{
    GtkStyleContext *context =
        gtk_widget_get_style_context(status_label);

    gtk_style_context_remove_class(context, "success");
    gtk_style_context_remove_class(context, "failure");
    gtk_style_context_remove_class(context, "neutral");

    gtk_style_context_add_class(context, css_class);

    gtk_label_set_text(
        GTK_LABEL(status_label),
        message
    );
}

/* ---------- Responsive Sudoku font ---------- */

static gboolean apply_font_idle(gpointer data)
{
    (void)data;

    if (pending_font_px == last_font_px)
        return G_SOURCE_REMOVE;

    last_font_px = pending_font_px;

    char css[100];

    g_snprintf(
        css,
        sizeof(css),
        "entry.sudoku-cell { font-size: %dpx; }",
        last_font_px
    );

    gtk_css_provider_load_from_data(
        cell_css,
        css,
        -1,
        NULL
    );

    return G_SOURCE_REMOVE;
}

static void on_grid_size_allocate(
    GtkWidget *widget,
    GdkRectangle *alloc,
    gpointer user_data
)
{
    (void)widget;
    (void)user_data;

    int side = MIN(alloc->width, alloc->height);
    int cell = side / SIZE;
    int font = CLAMP((int)(cell * 0.48), 10, 40);

    if (font != pending_font_px) {
        pending_font_px = font;
        g_idle_add(apply_font_idle, NULL);
    }
}

/* ---------- Thread status panel ---------- */

static const char *thread_status_text(int status)
{
    switch (status) {
        case THREAD_WAITING:
            return "WAITING";

        case THREAD_RUNNING:
            return "RUNNING";

        case THREAD_PASS:
            return "PASS";

        case THREAD_FAIL:
            return "FAIL";

        case THREAD_ERROR:
            return "ERROR";

        default:
            return "WAITING";
    }
}

static void refresh_thread_status_labels(void)
{
    for (int i = 0; i < TOTAL_THREADS; i++) {
        if (thread_status_labels[i] == NULL)
            continue;

        int state = get_thread_status(i);
        char text[100];

        if (i == 0) {
            g_snprintf(
                text,
                sizeof(text),
                "Мөрүүд: %s",
                thread_status_text(state)
            );
        } else if (i == 1) {
            g_snprintf(
                text,
                sizeof(text),
                "Баганууд: %s",
                thread_status_text(state)
            );
        } else {
            g_snprintf(
                text,
                sizeof(text),
                "Дэд хүснэгт %d: %s",
                i - 1,
                thread_status_text(state)
            );
        }

        gtk_label_set_text(
            GTK_LABEL(thread_status_labels[i]),
            text
        );
    }
}

static gboolean poll_thread_status(gpointer data)
{
    (void)data;

    refresh_thread_status_labels();

    if (validation_running)
        return G_SOURCE_CONTINUE;

    return G_SOURCE_REMOVE;
}

/* ---------- Validation worker ---------- */

static gboolean finish_validation_on_main(gpointer data)
{
    ValidationTask *task = (ValidationTask *)data;
    int validation = task->validation_result;

    validation_running = FALSE;

    refresh_thread_status_labels();

    gtk_widget_set_sensitive(check_button_widget, TRUE);
    gtk_widget_set_sensitive(clear_button_widget, TRUE);

    if (validation == -1) {
        set_status(
            "Алдаа: шалгах thread ажиллуулах боломжгүй байна.",
            "failure"
        );

        gtk_label_set_text(
            GTK_LABEL(result_label),
            "Thread үүсгэх эсвэл join хийхэд алдаа гарлаа."
        );

        g_free(task);
        return G_SOURCE_REMOVE;
    }

    if (validation == 1) {
        set_status(
            "Амжилттай! Sudoku зөв байна.",
            "success"
        );
    } else {
        set_status(
            "Sudoku буруу байна. FAIL үр дүнг шалгана уу.",
            "failure"
        );
    }

    GString *details = g_string_new("");

    g_string_append_printf(
        details,
        "Мөрүүд: %s\n",
        results[0] ? "PASS" : "FAIL"
    );

    g_string_append_printf(
        details,
        "Баганууд: %s\n",
        results[1] ? "PASS" : "FAIL"
    );

    g_string_append(
        details,
        "3×3 дэд хүснэгтүүд:\n"
    );

    for (int i = 0; i < 9; i++) {
        g_string_append_printf(
            details,
            "%s",
            results[i + 2] ? "PASS" : "FAIL"
        );

        if (i % 3 == 2)
            g_string_append(details, "\n");
        else
            g_string_append(details, "   ");
    }

    gtk_label_set_text(
        GTK_LABEL(result_label),
        details->str
    );

    g_string_free(details, TRUE);
    g_free(task);

    return G_SOURCE_REMOVE;
}

static gpointer validation_worker(gpointer data)
{
    ValidationTask *task = (ValidationTask *)data;

    task->validation_result = validate_sudoku();

    g_idle_add(
        finish_validation_on_main,
        task
    );

    return NULL;
}

/* ---------- Navigation ---------- */

static void on_start_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;
    (void)user_data;

    gtk_stack_set_visible_child_name(
        GTK_STACK(page_stack),
        "game"
    );
}

static void on_instructions_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;
    (void)user_data;

    gtk_stack_set_visible_child_name(
        GTK_STACK(page_stack),
        "instructions"
    );
}

/* ---------- Cell navigation ---------- */

static void on_cell_activate(
    GtkEntry *entry,
    gpointer user_data
)
{
    (void)entry;

    int index = GPOINTER_TO_INT(user_data);
    int next = (index + 1) % (SIZE * SIZE);

    GtkWidget *next_entry = gtk_grid_get_child_at(
        GTK_GRID(sudoku_grid),
        next % SIZE,
        next / SIZE
    );

    if (next_entry != NULL)
        gtk_widget_grab_focus(next_entry);
}

/* ---------- Check Sudoku ---------- */

static void on_check_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;
    (void)user_data;

    if (validation_running)
        return;

    gtk_label_set_text(
        GTK_LABEL(result_label),
        ""
    );

    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            GtkWidget *entry = gtk_grid_get_child_at(
                GTK_GRID(sudoku_grid),
                col,
                row
            );

            const char *text =
                gtk_entry_get_text(GTK_ENTRY(entry));

            if (text == NULL || text[0] == '\0') {
                char message[128];

                g_snprintf(
                    message,
                    sizeof(message),
                    "Алдаа: %d-р мөр, %d-р багана хоосон байна.",
                    row + 1,
                    col + 1
                );

                set_status(message, "failure");
                gtk_widget_grab_focus(entry);
                return;
            }

            if (strlen(text) != 1 ||
                !isdigit((unsigned char)text[0])) {
                char message[128];

                g_snprintf(
                    message,
                    sizeof(message),
                    "Алдаа: %d-р мөр, %d-р баганад 1–9 оруулна.",
                    row + 1,
                    col + 1
                );

                set_status(message, "failure");
                gtk_widget_grab_focus(entry);
                return;
            }

            int value = text[0] - '0';

            if (value < 1 || value > 9) {
                char message[128];

                g_snprintf(
                    message,
                    sizeof(message),
                    "Алдаа: %d-р мөр, %d-р баганад 1–9 оруулна.",
                    row + 1,
                    col + 1
                );

                set_status(message, "failure");
                gtk_widget_grab_focus(entry);
                return;
            }

            sudoku[row][col] = value;
        }
    }

    set_status(
        "Sudoku-г шалгаж байна...",
        "neutral"
    );

    gtk_widget_set_sensitive(
        check_button_widget,
        FALSE
    );

    gtk_widget_set_sensitive(
        clear_button_widget,
        FALSE
    );

    gtk_label_set_text(
        GTK_LABEL(result_label),
        "Шалгалт ажиллаж байна..."
    );

    validation_running = TRUE;

    refresh_thread_status_labels();

    g_timeout_add(
        50,
        poll_thread_status,
        NULL
    );

    ValidationTask *task = g_new0(
        ValidationTask,
        1
    );

    GThread *worker = g_thread_new(
        "sudoku-validator",
        validation_worker,
        task
    );

    if (worker == NULL) {
        validation_running = FALSE;

        gtk_widget_set_sensitive(
            check_button_widget,
            TRUE
        );

        gtk_widget_set_sensitive(
            clear_button_widget,
            TRUE
        );

        set_status(
            "Алдаа: шалгах thread үүсгэж чадсангүй.",
            "failure"
        );

        g_free(task);
        return;
    }

    g_thread_unref(worker);
}

/* ---------- Clear Sudoku ---------- */

static void on_clear_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;
    (void)user_data;

    if (validation_running)
        return;

    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            GtkWidget *entry = gtk_grid_get_child_at(
                GTK_GRID(sudoku_grid),
                col,
                row
            );

            if (entry != NULL)
                gtk_entry_set_text(GTK_ENTRY(entry), "");
        }
    }

    set_status(
        "Үр дүн: Шалгаагүй",
        "neutral"
    );

    gtk_label_set_text(
        GTK_LABEL(result_label),
        ""
    );

    refresh_thread_status_labels();
}

/* ---------- Instructions cards ---------- */

static GtkWidget *create_rule_card(
    const char *number,
    const char *heading,
    const char *description
)
{
    GtkWidget *card = gtk_box_new(
        GTK_ORIENTATION_HORIZONTAL,
        12
    );

    gtk_style_context_add_class(
        gtk_widget_get_style_context(card),
        "rule-card"
    );

    GtkWidget *number_label = make_label(
        number,
        "rule-number"
    );

    GtkWidget *text_box = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        4
    );

    GtkWidget *title = make_label(
        heading,
        "rule-text"
    );

    GtkWidget *detail = make_label(
        description,
        "rule-description"
    );

    gtk_box_pack_start(
        GTK_BOX(text_box),
        title,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(text_box),
        detail,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(card),
        number_label,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(card),
        text_box,
        TRUE,
        TRUE,
        0
    );

    return card;
}

static GtkWidget *create_instructions_content(void)
{
    GtkWidget *page = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        12
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(page),
        18
    );

    GtkWidget *title = make_label(
        "SUDOKU ТОГЛОХ ЗААВАР",
        NULL
    );

    gtk_widget_set_name(
        title,
        "welcome-title"
    );

    gtk_label_set_xalign(
        GTK_LABEL(title),
        0.5
    );

    GtkWidget *subtitle = make_label(
        "Дүрэмтэй танилцаад тоглоомоо эхлүүлээрэй.",
        NULL
    );

    gtk_widget_set_name(
        subtitle,
        "welcome-subtitle"
    );

    gtk_label_set_xalign(
        GTK_LABEL(subtitle),
        0.5
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        title,
        FALSE,
        FALSE,
        2
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        subtitle,
        FALSE,
        FALSE,
        2
    );

    GtkWidget *heading = make_label(
        "ХЭРХЭН ТОГЛОХ ВЭ?",
        "section-heading"
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        heading,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        create_rule_card(
            "1",
            "Нүд бүрд цифр оруулна",
            "9×9 хүснэгтийн нүд бүрд 1–9 хүртэлх нэг цифр бичнэ."
        ),
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        create_rule_card(
            "2",
            "Мөр ба багана",
            "Нэг мөр эсвэл нэг баганад ижил цифр давхардаж болохгүй."
        ),
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        create_rule_card(
            "3",
            "3×3 хэсэг",
            "Өнгөөр ялгасан 3×3 дэд хүснэгт бүрд цифр давхардахгүй."
        ),
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        create_rule_card(
            "4",
            "Хоосон нүд",
            "Нүд хоосон байвал програм тухайн нүдний байрлалыг мэдээлнэ."
        ),
        FALSE,
        FALSE,
        0
    );

    GtkWidget *notice = make_label(
        "САНАМЖ: PASS нь зөв, FAIL нь алдаа байгааг илэрхийлнэ.",
        "notice"
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        notice,
        FALSE,
        FALSE,
        0
    );

    return page;
}

/* ---------- Instructions page ---------- */
/* Scrollable content + fixed bottom button */

static GtkWidget *create_instructions_page(void)
{
    GtkWidget *page = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        0
    );

    GtkWidget *scroll = gtk_scrolled_window_new(
        NULL,
        NULL
    );

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC
    );

    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    gtk_container_add(
        GTK_CONTAINER(scroll),
        create_instructions_content()
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        scroll,
        TRUE,
        TRUE,
        0
    );

    /* This footer stays visible when the content scrolls. */
    GtkWidget *footer = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        6
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(footer),
        10
    );

    GtkWidget *start_button = make_button(
        "ТОГЛООМ ЭХЛЭХ",
        "primary-button"
    );

    gtk_widget_set_halign(
        start_button,
        GTK_ALIGN_FILL
    );

    gtk_box_pack_start(
        GTK_BOX(footer),
        start_button,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        footer,
        FALSE,
        FALSE,
        0
    );

    g_signal_connect(
        start_button,
        "clicked",
        G_CALLBACK(on_start_clicked),
        NULL
    );

    return page;
}

/* ---------- Thread panel ---------- */

static GtkWidget *create_thread_panel(void)
{
    GtkWidget *section = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        4
    );

    GtkWidget *heading = make_label(
        "УРСГАЛЫН ТӨЛӨВ",
        "rule-text"
    );

    gtk_box_pack_start(
        GTK_BOX(section),
        heading,
        FALSE,
        FALSE,
        0
    );

    GtkWidget *frame = gtk_frame_new(NULL);

    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(
        GTK_GRID(grid),
        4
    );

    gtk_grid_set_column_spacing(
        GTK_GRID(grid),
        6
    );

    gtk_grid_set_column_homogeneous(
        GTK_GRID(grid),
        TRUE
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(grid),
        6
    );

    for (int i = 0; i < TOTAL_THREADS; i++) {
        const char *initial_text;

        if (i == 0)
            initial_text = "Мөрүүд: WAITING";
        else if (i == 1)
            initial_text = "Баганууд: WAITING";
        else
            initial_text = "Дэд хүснэгт: WAITING";

        thread_status_labels[i] = make_label(
            initial_text,
            "thread-label"
        );

        gtk_label_set_line_wrap(
            GTK_LABEL(thread_status_labels[i]),
            TRUE
        );

        gtk_label_set_xalign(
            GTK_LABEL(thread_status_labels[i]),
            0.0
        );

        /*
         * Two columns reduce horizontal overflow.
         * The containing page can scroll vertically.
         */
        gtk_grid_attach(
            GTK_GRID(grid),
            thread_status_labels[i],
            i % 2,
            i / 2,
            1,
            1
        );
    }

    gtk_container_add(
        GTK_CONTAINER(frame),
        grid
    );

    gtk_box_pack_start(
        GTK_BOX(section),
        frame,
        FALSE,
        FALSE,
        0
    );

    return section;
}

/* ---------- Game page ---------- */
/* Entire page is scrollable on short windows */

static GtkWidget *create_game_page(void)
{
    GtkWidget *outer = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        0
    );

    GtkWidget *scroll = gtk_scrolled_window_new(
        NULL,
        NULL
    );

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC
    );

    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *page = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        6
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(page),
        10
    );

    GtkWidget *title = make_label(
        "Sudoku шалгах програм",
        "section-heading"
    );

    gtk_label_set_xalign(
        GTK_LABEL(title),
        0.5
    );

    GtkWidget *subtitle = make_label(
        "Нүд бүрд 1–9 хүртэлх нэг цифр оруулна.",
        "rule-description"
    );

    gtk_label_set_xalign(
        GTK_LABEL(subtitle),
        0.5
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        title,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        subtitle,
        FALSE,
        FALSE,
        0
    );

    /*
     * Square Sudoku grid.
     * Width follows the available window width.
     */
    GtkWidget *grid_frame = gtk_aspect_frame_new(
        NULL,
        0.5,
        0.5,
        1.0,
        FALSE
    );

    gtk_frame_set_shadow_type(
        GTK_FRAME(grid_frame),
        GTK_SHADOW_NONE
    );

    gtk_widget_set_hexpand(
        grid_frame,
        TRUE
    );

    gtk_widget_set_vexpand(
        grid_frame,
        FALSE
    );

    gtk_widget_set_size_request(
        grid_frame,
        280,
        280
    );

    sudoku_grid = gtk_grid_new();

    gtk_widget_set_hexpand(
        sudoku_grid,
        TRUE
    );

    gtk_widget_set_vexpand(
        sudoku_grid,
        TRUE
    );

    gtk_grid_set_row_spacing(
        GTK_GRID(sudoku_grid),
        2
    );

    gtk_grid_set_column_spacing(
        GTK_GRID(sudoku_grid),
        2
    );

    gtk_grid_set_row_homogeneous(
        GTK_GRID(sudoku_grid),
        TRUE
    );

    gtk_grid_set_column_homogeneous(
        GTK_GRID(sudoku_grid),
        TRUE
    );

    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            GtkWidget *entry = gtk_entry_new();

            gtk_entry_set_max_length(
                GTK_ENTRY(entry),
                1
            );

            gtk_entry_set_alignment(
                GTK_ENTRY(entry),
                0.5
            );

            gtk_entry_set_width_chars(
                GTK_ENTRY(entry),
                1
            );

            gtk_entry_set_max_width_chars(
                GTK_ENTRY(entry),
                1
            );

            gtk_widget_set_hexpand(
                entry,
                TRUE
            );

            gtk_widget_set_vexpand(
                entry,
                TRUE
            );

            GtkStyleContext *ctx =
                gtk_widget_get_style_context(entry);

            gtk_style_context_add_class(
                ctx,
                "sudoku-cell"
            );

            int block =
                (row / 3) * 3 + (col / 3);

            const char *class_name;

            if (block == 4)
                class_name = "block-c";
            else if (block % 2 == 0)
                class_name = "block-a";
            else
                class_name = "block-b";

            gtk_style_context_add_class(
                ctx,
                class_name
            );

            g_signal_connect(
                entry,
                "activate",
                G_CALLBACK(on_cell_activate),
                GINT_TO_POINTER(row * SIZE + col)
            );

            gtk_grid_attach(
                GTK_GRID(sudoku_grid),
                entry,
                col,
                row,
                1,
                1
            );
        }
    }

    gtk_container_add(
        GTK_CONTAINER(grid_frame),
        sudoku_grid
    );

    g_signal_connect(
        sudoku_grid,
        "size-allocate",
        G_CALLBACK(on_grid_size_allocate),
        NULL
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        grid_frame,
        FALSE,
        FALSE,
        2
    );

    /* Buttons */
    GtkWidget *buttons = gtk_box_new(
        GTK_ORIENTATION_HORIZONTAL,
        6
    );

    gtk_widget_set_halign(
        buttons,
        GTK_ALIGN_CENTER
    );

    check_button_widget = make_button(
        "Шалгах",
        "secondary-button"
    );

    clear_button_widget = make_button(
        "Цэвэрлэх",
        "secondary-button"
    );

    GtkWidget *back_button = make_button(
        "Заавар",
        "secondary-button"
    );

    gtk_box_pack_start(
        GTK_BOX(buttons),
        check_button_widget,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(buttons),
        clear_button_widget,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(buttons),
        back_button,
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        buttons,
        FALSE,
        FALSE,
        2
    );

    /* Main result */
    status_label = make_label(
        "Үр дүн: Шалгаагүй",
        "neutral"
    );

    gtk_label_set_xalign(
        GTK_LABEL(status_label),
        0.5
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        status_label,
        FALSE,
        FALSE,
        0
    );

    result_label = make_label(
        "",
        "neutral"
    );

    gtk_box_pack_start(
        GTK_BOX(page),
        result_label,
        FALSE,
        FALSE,
        0
    );

    /* Thread panel */
    gtk_box_pack_start(
        GTK_BOX(page),
        create_thread_panel(),
        FALSE,
        FALSE,
        2
    );

    gtk_container_add(
        GTK_CONTAINER(scroll),
        page
    );

    gtk_box_pack_start(
        GTK_BOX(outer),
        scroll,
        TRUE,
        TRUE,
        0
    );

    g_signal_connect(
        check_button_widget,
        "clicked",
        G_CALLBACK(on_check_clicked),
        NULL
    );

    g_signal_connect(
        clear_button_widget,
        "clicked",
        G_CALLBACK(on_clear_clicked),
        NULL
    );

    g_signal_connect(
        back_button,
        "clicked",
        G_CALLBACK(on_instructions_clicked),
        NULL
    );

    return outer;
}

/* ---------- Main ---------- */

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    setup_css();

    GtkWidget *window = gtk_window_new(
        GTK_WINDOW_TOPLEVEL
    );

    gtk_window_set_title(
        GTK_WINDOW(window),
        "Sudoku Validator"
    );

    gtk_window_set_default_size(
        GTK_WINDOW(window),
        760,
        720
    );

    /*
     * Smaller minimum size allows the window
     * to fit shorter displays.
     */
    gtk_widget_set_size_request(
        window,
        360,
        320
    );

    gtk_window_set_position(
        GTK_WINDOW(window),
        GTK_WIN_POS_CENTER
    );

    gtk_window_set_resizable(
        GTK_WINDOW(window),
        TRUE
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(window),
        4
    );

    page_stack = gtk_stack_new();

    gtk_widget_set_hexpand(
        page_stack,
        TRUE
    );

    gtk_widget_set_vexpand(
        page_stack,
        TRUE
    );

    gtk_stack_set_transition_type(
        GTK_STACK(page_stack),
        GTK_STACK_TRANSITION_TYPE_CROSSFADE
    );

    gtk_stack_set_transition_duration(
        GTK_STACK(page_stack),
        150
    );

    gtk_stack_add_named(
        GTK_STACK(page_stack),
        create_instructions_page(),
        "instructions"
    );

    gtk_stack_add_named(
        GTK_STACK(page_stack),
        create_game_page(),
        "game"
    );

    gtk_container_add(
        GTK_CONTAINER(window),
        page_stack
    );

    gtk_stack_set_visible_child_name(
        GTK_STACK(page_stack),
        "instructions"
    );

    g_signal_connect(
        window,
        "destroy",
        G_CALLBACK(gtk_main_quit),
        NULL
    );

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}