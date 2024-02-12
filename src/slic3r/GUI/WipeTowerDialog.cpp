#include <algorithm>
#include <sstream>
#include "libslic3r/FlushVolCalc.hpp"
#include "WipeTowerDialog.hpp"
#include "BitmapCache.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "libslic3r/Color.hpp"
#include "Widgets/Button.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include "MainFrame.hpp"

#include <wx/sizer.h>

using namespace Slic3r::GUI;

int scale(const int val) { return val * Slic3r::GUI::wxGetApp().em_unit() / 10; }
int ITEM_WIDTH() { return scale(30); }
static const wxColour g_text_color = wxColour(107, 107, 107, 255);

#define ICON_SIZE               wxSize(FromDIP(16), FromDIP(16))
#define TABLE_BORDER            FromDIP(28)
#define HEADER_VERT_PADDING     FromDIP(12)
#define HEADER_BEG_PADDING      FromDIP(30)
#define ICON_GAP                FromDIP(44)
#define HEADER_END_PADDING      FromDIP(24)
#define ROW_VERT_PADDING        FromDIP(6)
#define ROW_BEG_PADDING         FromDIP(20)
#define EDIT_BOXES_GAP          FromDIP(30)
#define ROW_END_PADDING         FromDIP(21)
#define BTN_SIZE                wxSize(FromDIP(58), FromDIP(24))
#define BTN_GAP                 FromDIP(20)
#define TEXT_BEG_PADDING        FromDIP(30)
#define MAX_FLUSH_VALUE         999
#define MIN_WIPING_DIALOG_WIDTH FromDIP(300)
#define TIP_MESSAGES_PADDING    FromDIP(8)



static void update_ui(wxWindow* window)
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(window);
}

RammingDialog::RammingDialog(wxWindow* parent,const std::string& parameters)
: wxDialog(parent, wxID_ANY, _(L("Ramming customization")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE/* | wxRESIZE_BORDER*/)
{
    update_ui(this);
    m_panel_ramming  = new RammingPanel(this,parameters);

    // Not found another way of getting the background colours of RammingDialog, RammingPanel and Chart correct than setting
    // them all explicitely. Reading the parent colour yielded colour that didn't really match it, no wxSYS_COLOUR_... matched
    // colour used for the dialog. Same issue (and "solution") here : https://forums.wxwidgets.org/viewtopic.php?f=1&t=39608
    // Whoever can fix this, feel free to do so.
#ifndef _WIN32
    this->           SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
    m_panel_ramming->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
#endif
    m_panel_ramming->Show(true);
    this->Show();

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_panel_ramming, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5);
    main_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 10);
    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    update_ui(static_cast<wxButton*>(this->FindWindowById(wxID_OK, this)));
    update_ui(static_cast<wxButton*>(this->FindWindowById(wxID_CANCEL, this)));

    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { EndModal(wxCANCEL); });

    this->Bind(wxEVT_BUTTON,[this](wxCommandEvent&) {
        m_output_data = m_panel_ramming->get_parameters();
        EndModal(wxID_OK);
        },wxID_OK);
    this->Show();
//    wxMessageDialog dlg(this, _(L("Ramming denotes the rapid extrusion just before a tool change in a single-extruder MM printer. Its purpose is to "
    Slic3r::GUI::MessageDialog dlg(this, _(L("Ramming denotes the rapid extrusion just before a tool change in a single-extruder MM printer. Its purpose is to "
        "properly shape the end of the unloaded filament so it does not prevent insertion of the new filament and can itself "
        "be reinserted later. This phase is important and different materials can require different extrusion speeds to get "
        "the good shape. For this reason, the extrusion rates during ramming are adjustable.\n\nThis is an expert-level "
        "setting, incorrect adjustment will likely lead to jams, extruder wheel grinding into filament etc.")), _(L("Warning")), wxOK | wxICON_EXCLAMATION);// .ShowModal();
    dlg.ShowModal();
}


#ifdef _WIN32
#define style wxSP_ARROW_KEYS | wxBORDER_SIMPLE
#else
#define style wxSP_ARROW_KEYS
#endif



RammingPanel::RammingPanel(wxWindow* parent, const std::string& parameters)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize/*,wxPoint(50,50), wxSize(800,350),wxBORDER_RAISED*/)
{
    update_ui(this);
	auto sizer_chart = new wxBoxSizer(wxVERTICAL);
	auto sizer_param = new wxBoxSizer(wxVERTICAL);

	std::stringstream stream{ parameters };
	stream >> m_ramming_line_width_multiplicator >> m_ramming_step_multiplicator;
	int ramming_speed_size = 0;
	float dummy = 0.f;
	while (stream >> dummy)
		++ramming_speed_size;
	stream.clear();
	stream.get();

	std::vector<std::pair<float, float>> buttons;
	float x = 0.f;
	float y = 0.f;
	while (stream >> x >> y)
		buttons.push_back(std::make_pair(x, y));

	m_chart = new Chart(this, wxRect(scale(10),scale(10),scale(480),scale(360)), buttons, ramming_speed_size, 0.25f, scale(10));
#ifdef _WIN32
    update_ui(m_chart);
#else
    m_chart->SetBackgroundColour(parent->GetBackgroundColour()); // see comment in RammingDialog constructor
#endif
 	sizer_chart->Add(m_chart, 0, wxALL, 5);

    m_widget_time						= new wxSpinCtrlDouble(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(ITEM_WIDTH()*2.5, -1),style,0.,5.0,3.,0.5);
    m_widget_volume							  = new wxSpinCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(ITEM_WIDTH()*2.5, -1),style,0,10000,0);
    m_widget_ramming_line_width_multiplicator = new wxSpinCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(ITEM_WIDTH()*2.5, -1),style,10,200,100);
    m_widget_ramming_step_multiplicator		  = new wxSpinCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(ITEM_WIDTH()*2.5, -1),style,10,200,100);

#ifdef _WIN32
    update_ui(m_widget_time->GetText());
    update_ui(m_widget_volume);
    update_ui(m_widget_ramming_line_width_multiplicator);
    update_ui(m_widget_ramming_step_multiplicator);
#endif

	auto gsizer_param = new wxFlexGridSizer(2, 5, 15);
	gsizer_param->Add(new wxStaticText(this, wxID_ANY, wxString(_(L("Total ramming time")) + " (" + _(L("s")) + "):")), 0, wxALIGN_CENTER_VERTICAL);
	gsizer_param->Add(m_widget_time);
	gsizer_param->Add(new wxStaticText(this, wxID_ANY, wxString(_(L("Total rammed volume")) + " (" + _(L("mm")) + wxString("³):", wxConvUTF8))), 0, wxALIGN_CENTER_VERTICAL);
	gsizer_param->Add(m_widget_volume);
	gsizer_param->AddSpacer(20);
	gsizer_param->AddSpacer(20);
	gsizer_param->Add(new wxStaticText(this, wxID_ANY, wxString(_(L("Ramming line width")) + " (%):")), 0, wxALIGN_CENTER_VERTICAL);
	gsizer_param->Add(m_widget_ramming_line_width_multiplicator);
	gsizer_param->Add(new wxStaticText(this, wxID_ANY, wxString(_(L("Ramming line spacing")) + " (%):")), 0, wxALIGN_CENTER_VERTICAL);
	gsizer_param->Add(m_widget_ramming_step_multiplicator);

	sizer_param->Add(gsizer_param, 0, wxTOP, scale(10));

    m_widget_time->SetValue(m_chart->get_time());
    m_widget_time->SetDigits(2);
    m_widget_volume->SetValue(m_chart->get_volume());
    m_widget_volume->Disable();
    m_widget_ramming_line_width_multiplicator->SetValue(m_ramming_line_width_multiplicator);
    m_widget_ramming_step_multiplicator->SetValue(m_ramming_step_multiplicator);
    
    m_widget_ramming_step_multiplicator->Bind(wxEVT_TEXT,[this](wxCommandEvent&) { line_parameters_changed(); });
    m_widget_ramming_line_width_multiplicator->Bind(wxEVT_TEXT,[this](wxCommandEvent&) { line_parameters_changed(); });

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(sizer_chart, 0, wxALL, 5);
	sizer->Add(sizer_param, 0, wxALL, 10);

	sizer->SetSizeHints(this);
	SetSizer(sizer);

    m_widget_time->Bind(wxEVT_TEXT,[this](wxCommandEvent&) {m_chart->set_xy_range(m_widget_time->GetValue(),-1);});
    m_widget_time->Bind(wxEVT_CHAR,[](wxKeyEvent&){});      // do nothing - prevents the user to change the value
    m_widget_volume->Bind(wxEVT_CHAR,[](wxKeyEvent&){});    // do nothing - prevents the user to change the value
    Bind(EVT_WIPE_TOWER_CHART_CHANGED,[this](wxCommandEvent&) {m_widget_volume->SetValue(m_chart->get_volume()); m_widget_time->SetValue(m_chart->get_time());} );
    Refresh(true); // erase background
}

void RammingPanel::line_parameters_changed() {
    m_ramming_line_width_multiplicator = m_widget_ramming_line_width_multiplicator->GetValue();
    m_ramming_step_multiplicator = m_widget_ramming_step_multiplicator->GetValue();
}

std::string RammingPanel::get_parameters()
{
    std::vector<float> speeds = m_chart->get_ramming_speed(0.25f);
    std::vector<std::pair<float,float>> buttons = m_chart->get_buttons();
    std::stringstream stream;
    stream << m_ramming_line_width_multiplicator << " " << m_ramming_step_multiplicator;
    for (const float& speed_value : speeds)
        stream << " " << speed_value;
    stream << "|";
    for (const auto& button : buttons)
        stream << " " << button.first << " " << button.second;
    return stream.str();
}


#ifdef _WIN32
#define style wxSP_ARROW_KEYS | wxBORDER_SIMPLE
#else
#define style wxSP_ARROW_KEYS
#endif

static const float g_min_flush_multiplier = 0.f;
static const float g_max_flush_multiplier = 3.f;

wxBoxSizer* WipingDialog::create_btn_sizer(long flags)
{
    auto btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer();

    StateColor ok_btn_bg(
        std::pair<wxColour, int>(wxColour(255, 174, 0  ), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(255, 174, 0  ), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 174, 0 ), StateColor::Normal)
    );

    StateColor ok_btn_bd(
        std::pair<wxColour, int>(wxColour(255, 174, 0 ), StateColor::Normal)
    );

    StateColor ok_btn_text(
        std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal)
    );

    StateColor cancel_btn_bg(
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal)
    );

    StateColor cancel_btn_bd_(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );

    StateColor cancel_btn_text(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );


    StateColor calc_btn_bg(
        std::pair<wxColour, int>(wxColour(255, 174, 0  ), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(255, 174, 0  ), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 174, 0 ), StateColor::Normal)
    );
    
    StateColor calc_btn_bd(
        std::pair<wxColour, int>(wxColour(255, 174, 0 ), StateColor::Normal)
    );
    
    StateColor calc_btn_text(
        std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal)
    );

    if (flags & wxRESET) {
        Button *calc_btn = new Button(this, _L("Recalculate"));
        calc_btn->SetMinSize(wxSize(FromDIP(75), FromDIP(24)));
        calc_btn->SetCornerRadius(FromDIP(12));
        calc_btn->SetBackgroundColor(calc_btn_bg);
        calc_btn->SetBorderColor(calc_btn_bd);
        calc_btn->SetTextColor(calc_btn_text);
        calc_btn->SetFocus();
        calc_btn->SetId(wxID_RESET);
        btn_sizer->Add(calc_btn, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);
        m_button_list[wxRESET] = calc_btn;
    }
    if (flags & wxOK) {
        Button* ok_btn = new Button(this, _L("OK"));
        ok_btn->SetMinSize(BTN_SIZE);
        ok_btn->SetCornerRadius(FromDIP(12));
        ok_btn->SetBackgroundColor(ok_btn_bg);
        ok_btn->SetBorderColor(ok_btn_bd);
        ok_btn->SetTextColor(ok_btn_text);
        ok_btn->SetFocus();
        ok_btn->SetId(wxID_OK);
        btn_sizer->Add(ok_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);
        m_button_list[wxOK] = ok_btn;
    }
    if (flags & wxCANCEL) {
        Button* cancel_btn = new Button(this, _L("Cancel"));
        cancel_btn->SetMinSize(BTN_SIZE);
        cancel_btn->SetCornerRadius(FromDIP(12));
        cancel_btn->SetBackgroundColor(cancel_btn_bg);
        cancel_btn->SetBorderColor(cancel_btn_bd_);
        cancel_btn->SetTextColor(cancel_btn_text);
        cancel_btn->SetId(wxID_CANCEL);
        btn_sizer->Add(cancel_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP / 2);
        m_button_list[wxCANCEL] = cancel_btn;
    }

    return btn_sizer;

}

void WipingDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    for (auto button_item : m_button_list)
    {
        if (button_item.first == wxRESET)
        {
            button_item.second->SetMinSize(wxSize(FromDIP(75), FromDIP(24)));
            button_item.second->SetCornerRadius(FromDIP(12));
        }
        if (button_item.first == wxOK) {
            button_item.second->SetMinSize(BTN_SIZE);
            button_item.second->SetCornerRadius(FromDIP(12));
        }
        if (button_item.first == wxCANCEL) {
            button_item.second->SetMinSize(BTN_SIZE);
            button_item.second->SetCornerRadius(FromDIP(12));
        }
    }
    m_panel_wiping->msw_rescale();
    this->Refresh();
};

// Parent dialog for purging volume adjustments - it fathers WipingPanel widget (that contains all controls) and a button to toggle simple/advanced mode:
WipingDialog::WipingDialog(wxWindow* parent, const std::vector<float>& matrix, const std::vector<float>& extruders, const std::vector<std::string>& extruder_colours,
    int extra_flush_volume, float flush_multiplier)
    : DPIDialog(parent ? parent : static_cast<wxWindow *>(wxGetApp().mainframe),
                wxID_ANY,
                _(L("Flushing volumes for filament change")),
                wxDefaultPosition,
                wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE /* | wxRESIZE_BORDER*/)
{
    std::string icon_path = (boost::format("%1%/images/logo_360.ico") % Slic3r::resources_dir()).str();
    SetIcon(wxIcon(Slic3r::encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));

    this->SetBackgroundColour(*wxWHITE);
    this->SetMinSize(wxSize(MIN_WIPING_DIALOG_WIDTH, -1));


    m_panel_wiping = new WipingPanel(this, matrix, extruders, extruder_colours, nullptr, extra_flush_volume, flush_multiplier);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_line_top, 0, wxEXPAND, 0);

    // set min sizer width according to extruders count
    auto sizer_width = (int)((sqrt(matrix.size()) + 2.8)*ITEM_WIDTH());
    sizer_width = sizer_width > MIN_WIPING_DIALOG_WIDTH ? sizer_width : MIN_WIPING_DIALOG_WIDTH;
    main_sizer->SetMinSize(wxSize(sizer_width, -1));
    main_sizer->Add(m_panel_wiping, 1, wxEXPAND | wxALL, 0);

    auto btn_sizer = create_btn_sizer(wxOK | wxCANCEL | wxRESET);
    main_sizer->Add(btn_sizer, 0, wxBOTTOM | wxRIGHT | wxEXPAND, BTN_GAP);
    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    if (this->FindWindowById(wxID_OK, this)) {
        this->FindWindowById(wxID_OK, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {                 // if OK button is clicked..
            m_output_matrix = m_panel_wiping->read_matrix_values();    // ..query wiping panel and save returned values
            EndModal(wxID_OK);
            }, wxID_OK);
    }
    if (this->FindWindowById(wxID_CANCEL, this)) {
        update_ui(static_cast<wxButton*>(this->FindWindowById(wxID_CANCEL, this)));
        this->FindWindowById(wxID_CANCEL, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxCANCEL); });

    }

    if (this->FindWindowById(wxID_RESET, this)) {
        this->FindWindowById(wxID_RESET, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { m_panel_wiping->calc_flushing_volumes(); });
    }

    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { EndModal(wxCANCEL); });
    this->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            if (this->IsModal())
                this->EndModal(wxID_CANCEL);
            else
                this->Close();
        }
        else
            e.Skip();
        });

    wxGetApp().UpdateDlgDarkUI(this);
}

void WipingPanel::create_panels(wxWindow* parent, const int num) {
    for (size_t i = 0; i < num; i++)
    {
        wxPanel* panel = new wxPanel(parent);
        panel->SetBackgroundColour(i % 2 == 0 ? *wxWHITE : wxColour(238, 238, 238));
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        panel->SetSizer(sizer);

        wxButton* icon = new wxButton(panel, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE, wxBORDER_NONE | wxBU_AUTODRAW);
        icon->SetBitmap(*get_extruder_color_icon(m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1), FromDIP(16), FromDIP(16)));
        icon->SetCanFocus(false);
        icon_list2.push_back(icon);

        sizer->AddStretchSpacer();
        sizer->AddSpacer(ROW_BEG_PADDING);
        sizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, ROW_VERT_PADDING);

        for (int j = 0; j < num; ++j) {
            edit_boxes[j][i]->Reparent(panel);
            edit_boxes[j][i]->SetBackgroundColour(panel->GetBackgroundColour());
            edit_boxes[j][i]->SetFont(::Label::Body_13);
            sizer->AddSpacer(EDIT_BOXES_GAP);
            sizer->Add(edit_boxes[j][i], 0, wxALIGN_CENTER_VERTICAL, 0);
        }
        sizer->AddSpacer(ROW_END_PADDING);
        sizer->AddStretchSpacer();

        m_sizer->Add(panel, 0, wxRIGHT | wxLEFT | wxEXPAND, TABLE_BORDER);
        panel->Layout();
    }
}

// This panel contains all control widgets for both simple and advanced mode (these reside in separate sizers)
WipingPanel::WipingPanel(wxWindow* parent, const std::vector<float>& matrix, const std::vector<float>& extruders, const std::vector<std::string>& extruder_colours, Button* calc_button,
    int extra_flush_volume, float flush_multiplier)
: wxPanel(parent,wxID_ANY, wxDefaultPosition, wxDefaultSize/*,wxBORDER_RAISED*/)
,m_matrix(matrix), m_min_flush_volume(extra_flush_volume), m_max_flush_volume(Slic3r::g_max_flush_volume)
{
    m_number_of_extruders = (int)(sqrt(matrix.size())+0.001);

    for (const std::string& color : extruder_colours) {
        Slic3r::ColorRGB rgb;
        Slic3r::decode_color(color, rgb);
        m_colours.push_back(wxColor(rgb.r_uchar(), rgb.g_uchar(), rgb.b_uchar()));
    }

    auto sizer_width = (int)((sqrt(matrix.size())) * ITEM_WIDTH() + (sqrt(matrix.size()) + 1) * HEADER_BEG_PADDING);
    sizer_width = sizer_width > MIN_WIPING_DIALOG_WIDTH ? sizer_width : MIN_WIPING_DIALOG_WIDTH;
    m_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetBackgroundColour(*wxWHITE);
    update_ui(this);

    // First create controls for advanced mode and assign them to m_page_advanced:
    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        edit_boxes.push_back(std::vector<wxTextCtrl*>(0));

        for (unsigned int j = 0; j < m_number_of_extruders; ++j) {
#ifdef _WIN32
            wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH(), -1), wxTE_CENTER | wxBORDER_NONE | wxTE_PROCESS_ENTER);
            update_ui(text);
            edit_boxes.back().push_back(text);
#else
            edit_boxes.back().push_back(new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH(), -1)));
#endif
            if (i == j) {
                edit_boxes[i][j]->SetValue(wxString("-"));
                edit_boxes[i][j]->SetEditable(false);
                edit_boxes[i][j]->Bind(wxEVT_KILL_FOCUS, [](wxFocusEvent&) {});
                edit_boxes[i][j]->Bind(wxEVT_SET_FOCUS, [](wxFocusEvent&) {});
            }
            else {
                edit_boxes[i][j]->SetValue(wxString("") << int(m_matrix[m_number_of_extruders * j + i] * flush_multiplier));

                edit_boxes[i][j]->Bind(wxEVT_TEXT, [this, i, j](wxCommandEvent& e) {
                    wxString str = edit_boxes[i][j]->GetValue();
                    int value = wxAtoi(str);
                    if (value > MAX_FLUSH_VALUE) {
                        str = wxString::Format(("%d"), MAX_FLUSH_VALUE);
                        edit_boxes[i][j]->SetValue(str);
                    }
                    });

                auto on_apply_text_modify = [this, i, j](wxEvent &e) {
                    wxString str   = edit_boxes[i][j]->GetValue();
                    int      value = wxAtoi(str);
                    m_matrix[m_number_of_extruders * j + i] = value / get_flush_multiplier();
                    this->update_warning_texts();
                    e.Skip();
                };

                edit_boxes[i][j]->Bind(wxEVT_TEXT_ENTER, on_apply_text_modify);
                edit_boxes[i][j]->Bind(wxEVT_KILL_FOCUS, on_apply_text_modify);
            }
        }
    }

    // BBS
    m_sizer->AddSpacer(FromDIP(10));
    auto tip_message_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    tip_message_panel->SetBackgroundColour(*wxWHITE);
    auto message_sizer = new wxBoxSizer(wxVERTICAL);
    tip_message_panel->SetSizer(message_sizer);
    {
        wxString message = _L("Orca recalculates your flushing volumes everytime the filament colors change. You can change this behavior in Preferences.");
        m_tip_message_label = new Label(tip_message_panel, wxEmptyString);
        wxClientDC dc(tip_message_panel);
        wxString multiline_message;
        m_tip_message_label->split_lines(dc, sizer_width, message, multiline_message);
        m_tip_message_label->SetLabel(multiline_message);
        m_tip_message_label->SetFont(Label::Body_13);
        message_sizer->Add(m_tip_message_label, 0, wxEXPAND | wxALL, TIP_MESSAGES_PADDING);
        update_ui(m_tip_message_label);
    }
    m_sizer->Add(tip_message_panel, 0, wxEXPAND | wxRIGHT | wxLEFT, TABLE_BORDER);
    bool is_show = wxGetApp().app_config->get("auto_calculate") == "true";
    tip_message_panel->Show(is_show);
    m_sizer->AddSpacer(FromDIP(10));

    m_sizer->AddSpacer(FromDIP(5));
    header_line_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    header_line_panel->SetBackgroundColour(wxColour(238, 238, 238));
    auto header_line_sizer = new wxBoxSizer(wxHORIZONTAL);
    header_line_panel->SetSizer(header_line_sizer);

    wxSizerItem* stretch_spacer = header_line_sizer->AddStretchSpacer();
    header_line_sizer->AddSpacer(HEADER_BEG_PADDING);
    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        wxButton* icon = new wxButton(header_line_panel, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE, wxBORDER_NONE | wxBU_AUTODRAW);
        icon->SetBitmap(*get_extruder_color_icon(m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1), FromDIP(16), FromDIP(16)));
        icon->SetCanFocus(false);
        icon_list1.push_back(icon);
        
        header_line_sizer->AddSpacer(ICON_GAP);
        header_line_sizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, HEADER_VERT_PADDING);
    }
    header_line_sizer->AddSpacer(HEADER_END_PADDING);
    header_line_sizer->AddStretchSpacer();

    m_sizer->Add(header_line_panel, 0, wxEXPAND | wxRIGHT | wxLEFT, TABLE_BORDER);

    create_panels(this, m_number_of_extruders);

    m_sizer->AddSpacer(FromDIP(5));

    // BBS: for tunning flush volumes
    {
        auto multi_desc_label = new wxStaticText(this, wxID_ANY, _(L("Flushing volume (mm³) for each filament pair.")), wxDefaultPosition, wxDefaultSize, 0);
        multi_desc_label->SetForegroundColour(g_text_color);
        m_sizer->Add(multi_desc_label, 0, wxEXPAND | wxLEFT, TEXT_BEG_PADDING);

        wxString min_flush_str = wxString::Format(_L("Suggestion: Flushing Volume in range [%d, %d]"), m_min_flush_volume, m_max_flush_volume);
        m_min_flush_label = new wxStaticText( this, wxID_ANY, min_flush_str, wxDefaultPosition, wxDefaultSize, 0);
        m_min_flush_label->SetForegroundColour(g_text_color);
        m_sizer->Add(m_min_flush_label, 0, wxEXPAND | wxLEFT, TEXT_BEG_PADDING);

        auto on_apply_text_modify = [this](wxEvent& e) {
            wxString str = m_flush_multiplier_ebox->GetValue();
            float      multiplier = wxAtof(str);
            if (multiplier < g_min_flush_multiplier || multiplier > g_max_flush_multiplier) {
                str = wxString::Format(("%.2f"), multiplier < g_min_flush_multiplier ? g_min_flush_multiplier : g_max_flush_multiplier);
                m_flush_multiplier_ebox->SetValue(str);
                MessageDialog dlg(nullptr,
                    wxString::Format(_L("The multiplier should be in range [%.2f, %.2f]."), g_min_flush_multiplier, g_max_flush_multiplier),
                    _L("Warning"), wxICON_WARNING | wxOK);
                dlg.ShowModal();
            }
            for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
                for (unsigned int j = 0; j < m_number_of_extruders; ++j) {
                    if (i == j)
                        continue; // if it is from/to the same extruder, don't change the value and continue
                    edit_boxes[i][j]->SetValue(to_string(int(m_matrix[m_number_of_extruders * j + i] * multiplier)));
                }
            }

            this->update_warning_texts();
            e.Skip();
        };

        m_sizer->AddSpacer(10);

        wxBoxSizer* param_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* flush_multiplier_title = new wxStaticText(this, wxID_ANY, _L("Multiplier"));
        param_sizer->Add(flush_multiplier_title, 0, wxALIGN_CENTER | wxALL, 0);
        param_sizer->AddSpacer(FromDIP(5));
        m_flush_multiplier_ebox = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(50), -1), wxTE_PROCESS_ENTER);
        char flush_multi_str[32] = { 0 };
        snprintf(flush_multi_str, sizeof(flush_multi_str), "%.2f", flush_multiplier);
        m_flush_multiplier_ebox->SetValue(flush_multi_str);
        param_sizer->Add(m_flush_multiplier_ebox, 0, wxALIGN_CENTER | wxALL, 0);
        param_sizer->AddStretchSpacer(1);
        m_sizer->Add(param_sizer, 0, wxEXPAND | wxLEFT, TEXT_BEG_PADDING);

        m_sizer->AddSpacer(BTN_SIZE.y);

        m_flush_multiplier_ebox->Bind(wxEVT_TEXT_ENTER, on_apply_text_modify);
        m_flush_multiplier_ebox->Bind(wxEVT_KILL_FOCUS, on_apply_text_modify);
    }
    this->update_warning_texts();

    m_sizer->SetSizeHints(this);
    SetSizer(m_sizer);
    this->Layout();

    header_line_panel->Bind(wxEVT_PAINT, [this, stretch_spacer](wxPaintEvent&) {
        wxPaintDC dc(header_line_panel);
        wxString from_text = _L("From");
        wxString to_text = _L("To");
        wxSize from_text_size = dc.GetTextExtent(from_text);
        wxSize to_text_size = dc.GetTextExtent(to_text);

        int base_y = (header_line_panel->GetSize().y - from_text_size.y - to_text_size.y) / 2;
        int vol_width = ROW_BEG_PADDING + EDIT_BOXES_GAP / 2 + ICON_SIZE.x;
        int base_x = stretch_spacer->GetSize().x + (vol_width - from_text_size.x - to_text_size.x) / 2;

        // draw from text
        int x = base_x;
        int y = base_y + to_text_size.y;
        dc.DrawText(from_text, x, y);

        // draw to text
        x = base_x + from_text_size.x;
        y = base_y;
        dc.DrawText(to_text, x, y);

        // draw a line
        int p1_x = base_x + from_text_size.x - to_text_size.y;
        int p1_y = base_y;
        int p2_x = base_x + from_text_size.x + from_text_size.y;
        int p2_y = base_y + from_text_size.y + to_text_size.y;
        dc.SetPen(wxPen(wxColour(172, 172, 172, 1)));
        dc.DrawLine(p1_x, p1_y, p2_x, p2_y);
    });
}

int WipingPanel::calc_flushing_volume(const wxColour& from_, const wxColour& to_)
{
    Slic3r::FlushVolCalculator calculator(m_min_flush_volume, m_max_flush_volume);

    return calculator.calc_flush_vol(from_.Alpha(), from_.Red(), from_.Green(), from_.Blue(), to_.Alpha(), to_.Red(), to_.Green(), to_.Blue());
}

void WipingPanel::update_warning_texts()
{
    static const wxColour g_warning_color = *wxRED;
    static const wxColour g_normal_color = *wxBLACK;

    wxString multi_str = m_flush_multiplier_ebox->GetValue();
    float multiplier = wxAtof(multi_str);

    bool has_exception_flush = false;
    for (int i = 0; i < edit_boxes.size(); i++) {
        auto& box_vec = edit_boxes[i];
        for (int j = 0; j < box_vec.size(); j++) {
            if (i == j)
                continue; // if it is from/to the same extruder, don't change the value and continue

            auto text_box = box_vec[j];
            wxString str = text_box->GetValue();
            int actual_volume = wxAtoi(str);
            if (actual_volume < m_min_flush_volume || actual_volume > m_max_flush_volume) {
                if (text_box->GetForegroundColour() != g_warning_color) {
                    text_box->SetForegroundColour(g_warning_color);
                    text_box->Refresh();
                }
                has_exception_flush = true;
            }
            else {
                if (text_box->GetForegroundColour() != g_normal_color) {
                    text_box->SetForegroundColour(StateColor::darkModeColorFor(g_normal_color));
                    text_box->Refresh();
                }
            }
        }
    }

    if (has_exception_flush && m_min_flush_label->GetForegroundColour() != g_warning_color) {
        m_min_flush_label->SetForegroundColour(g_warning_color);
        m_min_flush_label->Refresh();
    }
    else if (!has_exception_flush && m_min_flush_label->GetForegroundColour() != StateColor::darkModeColorFor(g_text_color)) {
        m_min_flush_label->SetForegroundColour(StateColor::darkModeColorFor(g_text_color));
        m_min_flush_label->Refresh();
    }
}

void WipingPanel::calc_flushing_volumes()
{
    for (int from_idx = 0; from_idx < m_colours.size(); from_idx++) {
        const wxColour& from = m_colours[from_idx];
        bool is_from_support = is_support_filament(from_idx);
        for (int to_idx = 0; to_idx < m_colours.size(); to_idx++) {
            if (from_idx == to_idx)
                continue; // if it is from/to the same extruder, don't change the value and continue

            bool is_to_support = is_support_filament(to_idx);
            int flushing_volume = 0;
            if (is_to_support) {
                flushing_volume = Slic3r::g_flush_volume_to_support;
            }
            else {
                const wxColour& to = m_colours[to_idx];
                flushing_volume = calc_flushing_volume(from, to);
                if (is_from_support) {
                    flushing_volume = std::max(Slic3r::g_min_flush_volume_from_support, flushing_volume);
                }
            }
            m_matrix[m_number_of_extruders * from_idx + to_idx] = flushing_volume;
            flushing_volume = int(flushing_volume * get_flush_multiplier());
            edit_boxes[to_idx][from_idx]->SetValue(std::to_string(flushing_volume));
        }
    }

    this->update_warning_texts();
}

void WipingPanel::msw_rescale()
{
    for (unsigned int i = 0; i < icon_list1.size(); ++i) {
        auto bitmap = *get_extruder_color_icon(m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1), FromDIP(16), FromDIP(16));
        icon_list1[i]->SetBitmap(bitmap);
        icon_list2[i]->SetBitmap(bitmap);
    }
}

// Reads values from the (advanced) wiping matrix:
std::vector<float> WipingPanel::read_matrix_values() {
    std::vector<float> output;
    for (unsigned int i=0;i<m_number_of_extruders;++i) {
        for (unsigned int j=0;j<m_number_of_extruders;++j) {
            double val = 0.;
            float  flush_multipler = get_flush_multiplier();
            if (flush_multipler == 0) {
                output.push_back(0.);
            }
            else {
                edit_boxes[j][i]->GetValue().ToDouble(&val);
                output.push_back((float) val / get_flush_multiplier());
            }
        }
    }
    return output;
}