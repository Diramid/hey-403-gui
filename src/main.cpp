#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string>
#include <iostream>
#include <unistd.h> // For geteuid() on Linux
#include <memory>
#include <cstdlib> // For system()

#ifdef _WIN32
#include <windows.h>
#endif

bool is_dns_set = false; // Track DNS state
GtkWidget *dns_label = nullptr; // Global pointer for the DNS label

// Function to get current DNS by running "hey403 -c"
std::string get_current_dns() {
    std::string dns = "Not set";
    FILE *pipe = popen("hey403 -c", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            dns = std::string(buffer);
            dns = dns.substr(0, dns.find('\n')); // Remove newline
        }
        pclose(pipe);
    }
    return dns;
}

// Update DNS label text
void update_dns_label() {
    std::string dns = get_current_dns();
    std::string label_text = "Current DNS: " + dns;
    gtk_label_set_text(GTK_LABEL(dns_label), label_text.c_str());
}

// Function to run system commands with sudo or PowerShell based on OS
int run_command(const std::string &command) {
#ifdef _WIN32
    // For Windows, use PowerShell to start a process with elevated privileges
    std::string elevated_command = "powershell Start-Process cmd -ArgumentList \"/c " + command + "\" -Verb runAs";
    return system(elevated_command.c_str());
#else
    // For Linux, use sudo
    return system(("sudo " + command).c_str());
#endif
}

// Struct to hold data for the deferred command execution
struct CommandData {
    GtkWidget *button;
    std::string command;
    bool set_dns;
    const char *domain;
};

// Callback to execute the command after UI update
static gboolean execute_command(gpointer user_data) {
    CommandData *data = static_cast<CommandData*>(user_data);
    GtkStyleContext *context = gtk_widget_get_style_context(data->button);

    int result = run_command(data->command);
    if (data->set_dns) { // Setting DNS
        if (result != 0) {
            g_print("Failed to set DNS. You may not have sufficient privileges.\n");
            gtk_button_set_label(GTK_BUTTON(data->button), "Set DNS");
            gtk_style_context_remove_class(context, "loading-button");
            gtk_style_context_add_class(context, "green-button");
        } else {
            gtk_button_set_label(GTK_BUTTON(data->button), "Stop DNS");
            gtk_style_context_remove_class(context, "loading-button");
            gtk_style_context_add_class(context, "red-button");
            is_dns_set = true;
            update_dns_label();
        }
    } else { // Unsetting DNS
        if (result != 0) {
            g_print("Failed to unset DNS. You may not have sufficient privileges.\n");
            gtk_button_set_label(GTK_BUTTON(data->button), "Stop DNS");
            gtk_style_context_remove_class(context, "loading-button");
            gtk_style_context_add_class(context, "red-button");
        } else {
            gtk_button_set_label(GTK_BUTTON(data->button), "Set DNS");
            gtk_style_context_remove_class(context, "loading-button");
            gtk_style_context_add_class(context, "green-button");
            is_dns_set = false;
            update_dns_label();
        }
    }
    gtk_widget_set_sensitive(data->button, TRUE); // Re-enable button only after completion
    gtk_widget_queue_draw(data->button);

    delete data; // Clean up allocated memory
    return G_SOURCE_REMOVE; // Remove this timeout
}

void on_button_clicked(GtkWidget *button, gpointer data) {
    GtkEntry *entry = GTK_ENTRY(data);
    const char *domain = gtk_entry_get_text(entry);
    GtkStyleContext *context = gtk_widget_get_style_context(button);

    // Set button to loading state and keep it disabled
    gtk_button_set_label(GTK_BUTTON(button), "Loading...");
    gtk_widget_set_sensitive(button, FALSE); // Disable button
    gtk_style_context_remove_class(context, "green-button");
    gtk_style_context_remove_class(context, "red-button");
    gtk_style_context_add_class(context, "loading-button");
    gtk_widget_queue_draw(button);

    // Process pending events to ensure UI updates
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    // Prepare command data
    CommandData *command_data = new CommandData{button, "", !is_dns_set, domain};
    if (!is_dns_set) { // Set DNS
        if (strlen(domain) == 0) {
            g_print("Please enter a domain.\n");
            gtk_button_set_label(GTK_BUTTON(button), "Set DNS");
            gtk_style_context_remove_class(context, "loading-button");
            gtk_style_context_add_class(context, "green-button");
            gtk_widget_set_sensitive(button, TRUE); // Re-enable button
            delete command_data;
            return;
        }
        command_data->command = "hey403 --set " + std::string(domain);
    } else { // Unset DNS
        command_data->command = "hey403 --unset";
    }

    // Defer the command execution to allow UI to update
    g_timeout_add(100, execute_command, command_data); // 100ms delay
}

int main(int argc, char **argv) {
    GtkWidget *window, *vbox, *hbox, *title, *entry, *button, *frame, *image;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Hey403App");
    gtk_window_set_default_size(GTK_WINDOW(window), 420, 350);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Horizontal box for image and title
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); // Increased spacing
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    // Load and scale the image
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file("../App/image.png", NULL);
    if (pixbuf) {
        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 100, 100, GDK_INTERP_BILINEAR);
        if (scaled_pixbuf) {
            image = gtk_image_new_from_pixbuf(scaled_pixbuf);
            gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
            g_object_unref(scaled_pixbuf);
        }
        g_object_unref(pixbuf);
    } else {
        g_print("Failed to load image.png\n");
    }

    // Title label (placed to the right of the image)
    title = gtk_label_new("Hey403: Forgot about 403 !");
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_size_new(22 * PANGO_SCALE));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(title), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(hbox), title, FALSE, FALSE, 2);

    // Input frame and entry field (vertical layout below)
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter domain...");
    gtk_container_add(GTK_CONTAINER(frame), entry);

    // DNS status label (placed directly below the title)
    dns_label = gtk_label_new("");  // Initialize DNS label
    PangoAttrList *dns_attrs = pango_attr_list_new();
    pango_attr_list_insert(dns_attrs, pango_attr_size_new(12 * PANGO_SCALE)); // Smaller font
    gtk_label_set_attributes(GTK_LABEL(dns_label), dns_attrs);
    pango_attr_list_unref(dns_attrs);
    gtk_box_pack_start(GTK_BOX(vbox), dns_label, FALSE, FALSE, 5); // Add DNS label below title
    update_dns_label(); // Set initial DNS value

    // Button setup: assign explicit name "dns-button"
    button = gtk_button_new_with_label("Set DNS");
    gtk_widget_set_size_request(button, 120, 40);
    gtk_widget_set_name(button, "dns-button");
    GtkStyleContext *btn_context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(btn_context, "green-button");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), entry);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 10);

    // CSS: using #dns-button selector to override system styles
    const char *css_data =
"#dns-button {"
"  color: #000000;"
"  background: #cccccc;"
"  padding: 10px 20px;"
"  border-radius: 10px;"
"  border: 2px solid #000000;"
"  -gtk-outline-radius: 10px;"
"  outline: none;"
"  font-weight: bold;"
"  text-shadow: none;"
"}"
"#dns-button.green-button {"
"  background: #28a745;"
"  border: 2px solid #1e7e34;"
"}"
"#dns-button.red-button {"
"  background: #dc3545;"
"  border: 2px solid #bd2130;"
"}"
"#dns-button.loading-button {"
"  background: #ffa500;" // Orange color for loading state
"  border: 2px solid #cc8400;"
"  color: #ffffff;" // White text for better contrast
"}"
"window {"
"  background: #1e1e2e;"
"  color: #ffffff;"
"}"
"label {"
"  color: #c9c9d1;"
"  text-shadow: none;"
"}"
"entry {"
"  background: #2e2e3e;"
"  color: #ffffff;"
"  border: 2px solid #6a0dad;"
"  border-radius: 5px;"
"  padding: 6px;"
"}"
"entry:focus {"
"  border-color: #5c2d91;"
"  outline: none;"
"}";

    GtkCssProvider *css_provider = gtk_css_provider_new();
    GError *css_error = NULL;
    if (!gtk_css_provider_load_from_data(css_provider, css_data, -1, &css_error)) {
        g_printerr("CSS Load Error: %s\n", css_error->message);
        g_error_free(css_error);
        g_object_unref(css_provider);
        return 1;
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 100
    );

    gtk_widget_show_all(window);
    gtk_main();
    g_object_unref(css_provider);
    return 0;
}
