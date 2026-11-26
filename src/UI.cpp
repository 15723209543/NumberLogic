#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <ctime>
#include <cwchar>
#include "UI.h"
#include "AI.h"
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
using namespace std;
namespace {
constexpr int W = 1160;
constexpr int H = 780;
constexpr UINT Tick = 1;
enum Id {
    Home = 101, Classic, Campaign, Duel, Practice, History, Statistics, Preferences, Help,
    Guess = 201, Submit, Hint, ClearInput, NewRound, NextLevel, Secret, StartDuel, AiStep,
    Records = 301, Export, DeleteRecords, Filter, Detail,
    Digits = 401, Attempts, Strength, Dark, SaveSettings,
    CopyTrace = 501
};
enum class Page { Home, Game, History, Stats, Settings, Help };
wstring wide(const string& s) {
    if (s.empty()) {
        return {};
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    wstring result(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), n);
    return result;
}
string narrow(const wstring& s) {
    if (s.empty()) {
        return {};
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    string result(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), n, nullptr, nullptr);
    return result;
}
wstring getdatapath() {
    PWSTR path=NULL;
    wstring result;
    if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,NULL,&path))) {
        result=path;
        CoTaskMemFree(path);
    } else {
        wchar_t temp[MAX_PATH];
        DWORD length=GetTempPathW(MAX_PATH,temp);
        if(length>0 && length<MAX_PATH) result=temp;
        else result=L".";
    }
    result+=L"\\NumberLogic";
    CreateDirectoryW(result.c_str(),NULL);
    return result;
}
wstring gettext(HWND control) {
    int n = GetWindowTextLengthW(control);
    wstring s(n + 1, L'\0');
    GetWindowTextW(control, s.data(), n + 1);
    s.resize(n);
    return s;
}
wstring number(int v) {
    return to_wstring(v);
}
class App {
    HWND window_ = nullptr;
    HINSTANCE instance_ = nullptr;
    HFONT font_ = nullptr;
    HFONT heading_ = nullptr;
    HFONT small_ = nullptr;
    HBRUSH background_ = nullptr;
    HBRUSH sidebar_ = nullptr;
    Page page_ = Page::Home;
    Mode mode_ = Mode::Classic;
    Round playergame;
    Round aigame;
    Solver aipanduan;
    Solver playerpanduan;
    Settings gamesetting;
    Store gamestore{getdatapath()};
    HWND allcontrol[128];
    int controlcount=0;
    HWND nav_[32];
    int navcount=0;
    Record gamerecord[recordmax];
    int recordcount=0;
    int shownumber[recordmax];
    int showcount=0;
    HWND status_ = nullptr;
    HWND clock_ = nullptr;
    HWND gameInfo_ = nullptr;
    HWND numberinput = nullptr;
    HWND gamehistory = nullptr;
    HWND aihistory = nullptr;
    HWND aiInfo_ = nullptr;
    HWND randnumbers = nullptr;
    HWND list_ = nullptr;
    HWND detail_ = nullptr;
    HWND filter_ = nullptr;
    bool duelready = false;
    bool duelfinished = false;
    bool issaved = false;
    bool proceed = false;
    bool aiwaiting = false;
    ULONGLONG starttime=0;
    COLORREF foreground() const {
        return gamesetting.dark ? RGB(230,237,248) : RGB(25,42,65);
    }
    COLORREF base() const {
        return gamesetting.dark ? RGB(25,35,50) : RGB(243,247,252);
    }
    COLORREF side() const {
        return gamesetting.dark ? RGB(17,25,40) : RGB(222,233,247);
    }
    // 切换主题时先释放旧画刷
    void palette() {
        if (background_) {
            DeleteObject(background_);
        }
        if (sidebar_) {
            DeleteObject(sidebar_);
        }
        background_ = CreateSolidBrush(base());
        sidebar_ = CreateSolidBrush(side());
        InvalidateRect(window_, nullptr, TRUE);
    }
    HWND control(const wchar_t* cls, const wstring& value, DWORD style,
                 int x, int y, int w, int h, int id = 0, bool nav = false, DWORD ex = 0) {
        HWND c = CreateWindowExW(ex, cls, value.c_str(), WS_CHILD | WS_VISIBLE | style,
            x, y, w, h, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
        if (c) {
            SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
            if(nav) { // 左侧导航单独保存
                if(navcount<32) nav_[navcount++]=c;
            } else {
                if(controlcount<128) allcontrol[controlcount++]=c;
            }
        }
        return c;
    }
    HWND label(const wstring& value, int x, int y, int w, int h, bool title = false) {
        HWND c = control(L"STATIC", value, SS_LEFT, x, y, w, h);
        if (title) {
            SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(heading_), TRUE);
        }
        return c;
    }
    HWND button(const wstring& value, int id, int x, int y, int w = 138, int h = 36, bool nav = false) {
        return control(L"BUTTON", value, BS_PUSHBUTTON | WS_TABSTOP, x, y, w, h, id, nav);
    }
    HWND edit(int id, int x, int y, int w, int h, DWORD flags = 0) {
        return control(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP | flags, x, y, w, h, id, false, WS_EX_CLIENTEDGE);
    }
    HWND memo(int x, int y, int w, int h, int id = 0) {
        return control(L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
                       x, y, w, h, id, false, WS_EX_CLIENTEDGE);
    }
    HWND combo(int id, int x, int y, int w, const wchar_t* items[],int itemcount,int selection) {
        HWND c = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, x, y, w, 250, id);
        for(int i=0;i<itemcount;i++) {
            SendMessageW(c, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(items[i]));
        }
        SendMessageW(c, CB_SETCURSEL, selection, 0);
        return c;
    }
    void text(HWND target, const wstring& value) {
        if (target) {
            SetWindowTextW(target, value.c_str());
        }
    }
    void status(const wstring& value) {
        text(status_, value);
    }
    void enable(int id, bool value) {
        HWND c = GetDlgItem(window_, id);
        if (c) {
            EnableWindow(c, value);
        }
    }
    int selected(int id) const {
        return static_cast<int>(SendDlgItemMessageW(window_, id, CB_GETCURSEL, 0, 0));
    }
    int elapsed() const {
        return (int)((GetTickCount64()-starttime)/1000);
    }
    // 换页只清除右侧控件
    void clearpage() {
        list_ = nullptr;
        for(int i=0;i<controlcount;i++) {
            DestroyWindow(allcontrol[i]);
        }
        controlcount=0;
        status_ = clock_ = gameInfo_ = numberinput = gamehistory = aihistory = aiInfo_ = randnumbers = list_ = detail_ = filter_ = nullptr;
    }
    void title(const wstring& name, const wstring& subtitle) {
        label(name, 218, 24, 810, 42, true);
        label(subtitle, 220, 74, 810, 30);
        status_ = label(L"就绪", 220, 638, 810, 35);
    }
    bool isleave() {
        if (page_ != Page::Game || !proceed || (mode_ == Mode::Duel ? duelfinished : playergame.state != State::Playing)) {
            return true;
        }
        return MessageBoxW(window_, L"当前游戏尚未完成，离开后本轮进度将丢失。继续吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES;
    }
    void nav() {
        control(L"STATIC", L"数字推理工坊", SS_LEFT, 16, 28, 184, 38, 0, true);
        int id[]={Home,Classic,Campaign,Duel,Practice,History,Statistics,Preferences,Help};
        const wchar_t* name[]={L"首页",L"经典挑战",L"进阶闯关",L"AI 推理对战",L"逻辑训练",L"历史与复盘",L"成绩统计",L"软件设置",L"帮助与关于"};
        int y=85;
        for(int i=0;i<9;i++) {
            button(name[i],id[i],16,y,176,42,true);
            y+=53; // 按钮间距
        }
    }
    void home() {
        label(L"欢迎来到数字推理工坊！", 245, 52, 850, 52, true);
        label(L"游戏规则", 245, 130, 850, 42, true);
        label(L"1. 系统随机生成一个四位数，每位数字为 1—9，且互不相同。", 245, 190, 850, 40);
        label(L"2. 玩家输入数字进行猜测，数字位数必须正确，不能含有 0 或重复数字。", 245, 248, 850, 55);
        label(L"3. 数字正确且位置正确，记为 1A；数字正确但位置不同，记为 1B。", 245, 315, 850, 55);
        label(L"例如：答案为 3579，猜测 2517，系统反馈 1A1B。", 265, 382, 830, 40);
        label(L"4. 根据每次反馈继续推理，经典模式最多有十次有效猜测机会。", 245, 440, 850, 55);
        label(L"5. 猜中全部数字即获胜，机会用完仍未猜中则失败。", 245, 507, 850, 40);
        label(L"6. 输入不符合规则时，可以重新输入，不消耗猜测机会。", 245, 565, 850, 55);
    }
    void start(Mode mode) {
        mode_ = mode;
        int d = mode == Mode::Practice ? gamesetting.digit : 4;
        int a = mode == Mode::Practice ? gamesetting.attempts : 10;
        playergame.start(mode, d, a);
        playerpanduan.reset(d);
        duelready = false;
        duelfinished = false;
        aiwaiting = false;
        proceed = mode != Mode::Duel;
        issaved = false;
        starttime = GetTickCount64();
        if (mode == Mode::Duel) {
            aipanduan.reset(d);
        }
    }
    void game() {
        title(wide(getmodename(mode_)) + L"模式", L"A：数字与位置均正确；B：数字正确但位置不同。");
        gameInfo_ = label(L"", 220, 115, 790, 36, true);
        clock_ = label(L"", 850, 115, 170, 28);
        label(L"输入猜测", 220, 165, 130, 32);
        numberinput = edit(Guess, 345, 160, 210, 36);
        SendMessageW(numberinput, EM_SETLIMITTEXT, 32, 0);
        button(L"提交猜测", Submit, 570, 160, 125);
        button(L"清空", ClearInput, 705, 160, 95);
        button(L"重新开始", NewRound, 810, 160, 170);
        if (mode_ == Mode::Duel) {
            label(L"设置给 AI 的谜底", 220, 218, 175, 30);
            randnumbers = edit(Secret, 400, 212, 155, 35, ES_PASSWORD);
            SendMessageW(randnumbers, EM_SETLIMITTEXT, 9, 0);
            button(L"开始对战", StartDuel, 570, 212, 125);
            button(L"AI 回合", AiStep, 705, 212, 125);
            label(L"玩家记录", 220, 265, 365, 32);
            label(L"AI 推理记录", 635, 265, 365, 32);
            gamehistory = memo(220, 300, 390, 265);
            aihistory = memo(635, 300, 390, 265);
            aiInfo_ = label(L"等待对战开始", 635, 580, 390, 40);
        } else {
            button(L"智能提示", Hint, 220, 218, 155);
            button(L"下一关", NextLevel, 390, 218, 155);
            button(L"复制复盘", CopyTrace, 560, 218, 155);
            gamehistory = memo(220, 275, 805, 330);
        }
        refreshgame();
        status(mode_ == Mode::Duel ? L"请先输入一个四位谜底，AI 不会直接读取它作为推理输入。" : L"请输入符合规则的数字。无效输入不消耗次数。");
        SetFocus(mode_ == Mode::Duel ? randnumbers : numberinput);
    }
    wstring getlogtext(const Round& r) const {
        wstring out = L"序号        猜测             反馈\r\n";
        out += L"------------------------------------------\r\n";
        int i = 0;
        for(int j=0;j<r.count;j++) {
            const Attempt& a=r.history[j];
            out += number(++i) + L"             " + wide(a.guess) + L"             " + wide(a.result.str()) + L"\r\n";
        }
        if (r.state != State::Playing) {
            out += L"\r\n正确答案：" + wide(r.reveal()) + L"\r\n";
            out += r.state == State::Won ? L"本轮成功" : L"机会用尽";
        }
        return out;
    }
    // 更新剩余次数和按钮状态
    void refreshgame() {
        text(gameInfo_, L"第 " + number(playergame.level) + L" 关   |   " + number(playergame.digit) + L" 位   |   剩余 " + number(playergame.getcount()) + L" 次");
        text(gamehistory, getlogtext(playergame));
        if (mode_ == Mode::Duel) {
            if (duelready) {
                text(aihistory, getlogtext(aigame));
                text(aiInfo_, L"AI 剩余候选：" + number(static_cast<int>(aipanduan.size())) + L"   已猜：" + number(aigame.count));
            }
            enable(StartDuel, !duelready);
            enable(Secret, !duelready);
            enable(AiStep, duelready && aiwaiting && !duelfinished);
        }
        enable(Submit, proceed && playergame.state == State::Playing && !aiwaiting && !duelfinished);
        enable(Guess, proceed && playergame.state == State::Playing && !aiwaiting && !duelfinished);
        enable(Hint, mode_ != Mode::Duel && playergame.state == State::Playing);
        enable(NextLevel, mode_ == Mode::Campaign && playergame.state == State::Won);
    }
    void savegame() {
        if (issaved || playergame.state == State::Playing) { // 一局只保存一次
            return;
        }
        Record r;
        r.date = gettime();
        r.mode = getmodename(mode_);
        r.digit = playergame.digit;
        r.level = playergame.level;
        r.count = playergame.count;
        r.maxcount = playergame.maxcount;
        r.won = playergame.state == State::Won;
        r.seconds = elapsed();
        r.secret = playergame.reveal();
        r.gethistory = gethistory(playergame.history,playergame.count);
        issaved = gamestore.append(r);
        if (!issaved) {
            MessageBoxW(window_, L"写入本地历史失败，请检查存储权限。", L"保存失败", MB_OK | MB_ICONWARNING);
        }
    }
    void submit() {
        if (!proceed || aiwaiting || duelfinished) {
            return;
        }
        string error = playergame.submit(narrow(gettext(numberinput)));
        if (!error.empty()) {
            status(wide(error) + L"，请重新输入。");
            return;
        }
        playerpanduan.removenumber(playergame.history[playergame.count-1]);
        text(numberinput, L"");
        if (mode_ == Mode::Duel) {
            aiwaiting = true;
            status(L"玩家回合已完成，请点击 AI 回合。");
        } else if (playergame.state == State::Won) {
            savegame();
            status(mode_ == Mode::Campaign ? L"挑战成功！点击下一关继承剩余机会。" : L"恭喜，挑战成功！");
        } else if (playergame.state == State::Lost) {
            savegame();
            status(L"挑战失败，正确答案已显示。");
        } else {
            status(L"反馈：" + wide(playergame.history[playergame.count-1].result.str()) + L"；继续推理。");
        }
        refreshgame();
        if (IsWindowEnabled(numberinput)) {
            SetFocus(numberinput);
        }
    }
    void startduel() {
        if (duelready) {
            return;
        }
        string secret = narrow(gettext(randnumbers));
        string error = checknumber(secret, 4);
        if (!error.empty()) {
            status(wide(error));
            return;
        }
        aigame.start(Mode::Duel, 4, 10, 1, secret);
        aipanduan.reset(4);
        duelready = proceed = true;
        starttime = GetTickCount64();
        text(randnumbers, L"");
        status(L"对战开始。玩家先猜，AI 随后猜测。");
        refreshgame();
        SetFocus(numberinput);
    }
    void aiguess() {
        if (!duelready || !aiwaiting || duelfinished) {
            return;
        }
        string choice = aipanduan.guessnumber(gamesetting.strength);
        if (choice.empty()) {
            status(L"AI 候选集为空，推理状态异常。");
            return;
        }
        string error = aigame.submit(choice);
        if (!error.empty()) {
            status(wide(error));
            return;
        }
        aipanduan.removenumber(aigame.history[aigame.count-1]);
        aiwaiting = false; // AI本轮已猜完
        if (playergame.state != State::Playing || aigame.state != State::Playing) {
            duelfinished = true;
            bool playerWon = playergame.state == State::Won;
            bool aiWon = aigame.state == State::Won;
            if (playerWon && aiWon) {
                status(L"双方同一轮猜中，平局。");
            } else if (playerWon) {
                status(L"玩家获胜！");
            } else if (aiWon) {
                status(L"AI 获胜！");
            } else {
                status(L"双方机会用尽，平局。");
            }
            // 对战结束时记录玩家的实际猜测次数。
            if (playergame.state == State::Playing) {
                playergame.state = State::Lost;
            }
            if (aigame.state == State::Playing) {
                aigame.state = State::Lost;
            }
            savegame();
        } else {
            status(L"AI 猜测 " + wide(choice) + L"，反馈 " + wide(aigame.history[aigame.count-1].result.str()) + L"。轮到玩家。");
        }
        refreshgame();
    }
    void hint() {
        if (mode_ == Mode::Duel || playergame.state != State::Playing) {
            return;
        }
        string choice = playerpanduan.guessnumber(gamesetting.strength);
        status(L"剩余候选 " + number(static_cast<int>(playerpanduan.size())) + L" 个；建议尝试 " + wide(choice) + L"（不保证正确）。");
    }
    void next() {
        if (!playergame.next()) {
            return;
        }
        playerpanduan.reset(playergame.digit);
        issaved = false;
        starttime = GetTickCount64();
        refreshgame();
        status(L"下一关已开始，剩余机会已继承。");
        SetFocus(numberinput);
    }
    void copy(const wstring& value) {
        if (!OpenClipboard(window_)) {
            status(L"无法打开剪贴板。");
            return;
        }
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (value.size() + 1) * sizeof(wchar_t));
        if (!mem) {
            CloseClipboard();
            return;
        }
        wchar_t* ptr = static_cast<wchar_t*>(GlobalLock(mem));
        if (!ptr) {
            GlobalFree(mem);
            CloseClipboard();
            return;
        }
        for(size_t i=0;i<=value.size();i++) ptr[i]=value[i];
        GlobalUnlock(mem);
        EmptyClipboard();
        if (!SetClipboardData(CF_UNICODETEXT, mem)) {
            GlobalFree(mem);
            status(L"复制失败。");
        } else {
            status(L"已复制到剪贴板。");
        }
        CloseClipboard();
    }
    void history() {
        title(L"历史记录与复盘", L"查看本地完成的对局，按模式筛选并导出 CSV。");
        label(L"模式筛选", 220, 122, 100, 30);
        const wchar_t* filters[]={L"全部",L"经典",L"闯关",L"AI对战",L"训练"};
        filter_=combo(Filter,325,115,155,filters,5,0);
        button(L"导出 CSV", Export, 510, 115, 155);
        button(L"清空历史", DeleteRecords, 685, 115, 155);
        list_ = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_TABSTOP | WS_BORDER,
                        220, 165, 805, 270, Records);
        ListView_SetExtendedListViewStyle(list_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        const wchar_t* names[] = {L"时间",L"模式",L"位数",L"关卡",L"次数",L"结果",L"耗时"};
        int widths[] = {185,110,75,75,95,95,95};
        for (int i = 0; i < 7; ++i) {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = const_cast<wchar_t*>(names[i]);
            col.cx = widths[i];
            ListView_InsertColumn(list_, i, &col);
        }
        label(L"选中记录查看详细过程", 220, 450, 805, 30);
        detail_ = memo(220, 480, 805, 140, Detail);
        recordcount=gamestore.load(gamerecord,recordmax);
        populate();
    }
    void populate() {
        ListView_DeleteAllItems(list_);
        showcount=0;
        int f = selected(Filter);
        string expected;
        if (f > 0 && f <= 4) {
            expected = getmodename(static_cast<Mode>(f - 1));
        }
        // 最新的记录放在最上面
        for (size_t offset = recordcount; offset > 0; --offset) {
            size_t i = offset - 1;
            const Record& r = gamerecord[i];
            if (!expected.empty() && r.mode != expected) {
                continue;
            }
            int row = static_cast<int>(showcount);
            shownumber[showcount++]=(int)i; // 保存原记录下标
            wstring values[] = {wide(r.date),wide(r.mode),number(r.digit),number(r.level),
                number(r.count)+L"/"+number(r.maxcount),r.won?L"成功":L"未成功",number(r.seconds)+L" 秒"};
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.pszText = values[0].data();
            ListView_InsertItem(list_, &item);
            for (int c = 1; c < 7; ++c) {
                ListView_SetItemText(list_, row, c, values[c].data());
            }
        }
        text(detail_, L"");
        status(L"当前显示 " + number(static_cast<int>(showcount)) + L" 条记录。");
    }
    void showdetail() {
        int i = ListView_GetNextItem(list_, -1, LVNI_SELECTED);
        if (i < 0 || static_cast<size_t>(i) >= showcount) {
            return;
        }
        const Record& r = gamerecord[shownumber[i]];
        wstring result = L"答案：" + wide(r.secret) + L"    时间：" + wide(r.date) + L"\r\n";
        string token;
        int index=0;
        for(size_t j=0;j<=r.gethistory.size();j++) {
            if(j==r.gethistory.size() || r.gethistory[j]==';') {
                if(!token.empty()) result+=L"第 "+number(++index)+L" 次："+wide(token)+L"\r\n";
                token="";
            } else token+=r.gethistory[j];
        }
        text(detail_, result);
    }
    void exporthistory() {
        wchar_t path[MAX_PATH] = L"NumberLogic-history.csv";
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = window_;
        dialog.lpstrFile = path;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrFilter = L"CSV 文件 (*.csv)\0*.csv\0\0";
        dialog.lpstrDefExt = L"csv";
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetSaveFileNameW(&dialog)) {
            status(gamestore.exportcsv(path) ? L"已导出全部历史记录。" : L"导出失败，请检查文件权限。");
        }
    }
    void deletehistory() {
        if (MessageBoxW(window_, L"永久删除所有历史记录？", L"确认清空", MB_YESNO | MB_ICONWARNING) != IDYES) {
            return;
        }
        if (gamestore.clear()) {
            recordcount=0;
            populate();
            status(L"历史已清空。");
        } else {
            status(L"删除失败。");
        }
    }
    void stats() {
        title(L"成绩统计", L"按最近2000局完成对局统计，不含中途退出的记录。");
        recordcount=gamestore.load(gamerecord,recordmax);
        Stats s=countresult(gamerecord,recordcount);
        int percent = s.rounds ? 100 * s.wins / s.rounds : 0;
        label(L"完成局数  " + number(s.rounds), 235, 145, 360, 45, true);
        label(L"猜中局数  " + number(s.wins), 650, 145, 360, 45, true);
        label(L"猜中率  " + number(percent) + L"%", 235, 215, 360, 45, true);
        label(L"最少猜测  " + number(s.best), 650, 215, 360, 45, true);
        wchar_t mean[32];
        swprintf(mean,32,L"%.1f",s.mean);
        label(wstring(L"成功局平均次数  ")+mean, 235, 285, 390, 40);
        label(L"最高成功关卡  " + number(s.highest), 650, 285, 360, 40);
        label(L"各模式猜中率", 235, 350, 790, 35, true);
        int y = 400;
        Mode modes[]={Mode::Classic,Mode::Campaign,Mode::Duel,Mode::Practice};
        for(int i=0;i<4;i++) {
            Mode m=modes[i];
            Stats stat=countresult(gamerecord,recordcount,getmodename(m));
            int rate = stat.rounds ? 100 * stat.wins / stat.rounds : 0;
            label(wide(getmodename(m)), 235, y, 125, 30);
            HWND bar = control(PROGRESS_CLASSW, L"", PBS_SMOOTH, 365, y+3, 420, 22);
            SendMessageW(bar, PBM_SETRANGE32, 0, 100);
            SendMessageW(bar, PBM_SETPOS, rate, 0);
            label(number(stat.wins) + L"/" + number(stat.rounds) + L"  (" + number(rate) + L"%)", 805, y, 205, 30);
            y += 48;
        }
    }
    void preferences() {
        title(L"软件设置", L"训练模式参数和 AI 难度在本机持久化保存。");
        label(L"训练数字位数", 235, 150, 250, 35);
        const wchar_t* digits[]={L"3 位",L"4 位",L"5 位",L"6 位"};
        combo(Digits,500,145,250,digits,4,gamesetting.digit-3);
        label(L"训练机会次数", 235, 220, 250, 35);
        HWND limits=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,500,215,250,250,Attempts);
        for(int i=5;i<=20;i++) {
            wstring value=number(i)+L" 次";
            SendMessageW(limits,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value.c_str()));
        }
        SendMessageW(limits,CB_SETCURSEL,gamesetting.attempts-5,0);
        label(L"AI 推理策略", 235, 290, 250, 35);
        const wchar_t* strengths[]={L"基础：候选顺序",L"进阶：期望剩余最小",L"高级：兼顾最坏分组"};
        combo(Strength,500,285,350,strengths,3,gamesetting.strength-1);
        HWND check = control(L"BUTTON", L"深色页面背景", BS_AUTOCHECKBOX | WS_TABSTOP, 235, 360, 300, 35, Dark);
        SendMessageW(check, BM_SETCHECK, gamesetting.dark ? BST_CHECKED : BST_UNCHECKED, 0);
        button(L"保存设置", SaveSettings, 235, 435, 200, 45);
        label(L"数据目录：", 235, 510, 180, 30);
        label(gamestore.root(), 235, 550, 780, 65);
    }
    void savesetting() {
        gamesetting.digit = limitnumber(selected(Digits)+3, 3, 6);
        gamesetting.attempts = limitnumber(selected(Attempts)+5, 5, 20);
        gamesetting.strength = limitnumber(selected(Strength)+1, 1, 3);
        gamesetting.dark = SendDlgItemMessageW(window_, Dark, BM_GETCHECK, 0, 0) == BST_CHECKED;
        bool ok = gamesetting.save(gamestore.root()+L"/settings.txt");
        palette();
        status(ok ? L"设置已保存，下次启动自动读取。" : L"设置无法保存到磁盘。");
    }
    void help() {
        title(L"帮助与关于", L"数字推理工坊 V1.0.0 · Windows 本地逻辑训练软件");
        HWND content = memo(220, 120, 805, 485);
        text(content, L"【基础规则】\r\n系统从 1 至 9 选取互不相同的数字。\r\nA 表示数字正确且位置正确，B 表示数字正确但位置错误。\r\n例如答案 3579，猜测 2517，反馈 1A1B。\r\n有效猜测记录一次，错误格式不消耗；闯关猜中时继承机会不扣除该次。\r\n\r\n【四种模式】\r\n经典：固定四位，十次机会。\r\n闯关：每次成功增加一位，最高九位，继承剩余机会并补充十次（最多1024次）。\r\n训练：自定义位数和机会，支持候选提示。\r\n对战：输入自己的谜底后，与 AI 交替猜测。双方同轮猜中判平。\r\n\r\n【离线 AI】\r\nAI 枚举所有合法排列，根据每一次 A/B 反馈筛除不一致候选。\r\n进阶策略比较反馈分组，降低期望剩余候选数；高级兼顾最大分组。\r\n为控制运算量，策略使用确定性抽样，不保证全局最优。\r\n\r\n【数据说明】\r\n历史页面和统计最多显示最近2000局，CSV导出全部。历史仅保存完成的玩家回合；AI 对战记录中的成功表示玩家猜中，非对战胜负。\r\n历史和设置保存在当前 Windows 用户 LocalAppData 下。\r\nCSV 导出包含正确答案及猜测过程。\r\n");
    }
    void navigate(Page page, Mode mode = Mode::Classic) {
        if (!isleave()) {
            return;
        }
        proceed = false;
        clearpage();
        page_ = page;
        switch (page) {
            case Page::Home: home(); break;
            case Page::Game: start(mode); game(); break;
            case Page::History: history(); break;
            case Page::Stats: stats(); break;
            case Page::Settings: preferences(); break;
            case Page::Help: help(); break;
        }
        InvalidateRect(window_, nullptr, TRUE);
    }
    void command(int id, int notification) {
        if (id == Filter && notification == CBN_SELCHANGE) {
            populate();
            return;
        }
        if (notification != BN_CLICKED) {
            return;
        }
        switch (id) {
            case Home: navigate(Page::Home); break;
            case Classic: navigate(Page::Game, Mode::Classic); break;
            case Campaign: navigate(Page::Game, Mode::Campaign); break;
            case Duel: navigate(Page::Game, Mode::Duel); break;
            case Practice: navigate(Page::Game, Mode::Practice); break;
            case History: navigate(Page::History); break;
            case Statistics: navigate(Page::Stats); break;
            case Preferences: navigate(Page::Settings); break;
            case Help: navigate(Page::Help); break;
            case Submit: submit(); break;
            case Hint: hint(); break;
            case ClearInput: text(numberinput, L""); SetFocus(numberinput); break;
            case NewRound: navigate(Page::Game, mode_); break;
            case NextLevel: next(); break;
            case StartDuel: startduel(); break;
            case AiStep: aiguess(); break;
            case Export: exporthistory(); break;
            case DeleteRecords: deletehistory(); break;
            case SaveSettings: savesetting(); break;
            case CopyTrace: copy(getlogtext(playergame)); break;
        }
    }
public:
    explicit App(HINSTANCE instance) : instance_(instance) {}
    void attach(HWND window) {
        window_ = window;
    }
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CREATE:
                gamesetting.load(gamestore.root()+L"/settings.txt");
                font_ = CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
                heading_ = CreateFontW(-29,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
                palette();
                nav();
                home();
                SetTimer(window_, Tick, 1000, nullptr);
                return 0;
            case WM_COMMAND:
                command(LOWORD(wp), HIWORD(wp));
                return 0;
            case WM_NOTIFY:
                if (list_ && reinterpret_cast<NMHDR*>(lp)->idFrom == Records && reinterpret_cast<NMHDR*>(lp)->code == LVN_ITEMCHANGED) {
                    showdetail();
                }
                return 0;
            case WM_TIMER: // 每秒刷新一次耗时
                if (page_ == Page::Game && proceed && (mode_ == Mode::Duel ? !duelfinished : playergame.state == State::Playing)) {
                    text(clock_, L"耗时 " + number(elapsed()) + L" 秒");
                }
                return 0;
            case WM_CTLCOLORSTATIC: {
                HDC dc = reinterpret_cast<HDC>(wp);
                HWND c = reinterpret_cast<HWND>(lp);
                SetTextColor(dc, foreground());
                bool isNav=false;
                for(int i=0;i<navcount;i++) {
                    if(nav_[i]==c) isNav=true;
                }
                if (c == gamehistory || c == aihistory || c == detail_) {
                    SetTextColor(dc, RGB(20,35,55));
                    SetBkColor(dc, RGB(255,255,255));
                    return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
                }
                SetBkColor(dc, isNav ? side() : base());
                return reinterpret_cast<LRESULT>(isNav ? sidebar_ : background_);
            }
            case WM_ERASEBKGND: {
                HDC dc = reinterpret_cast<HDC>(wp);
                RECT r{};
                GetClientRect(window_, &r);
                FillRect(dc,&r,background_);
                r.right = 207; // 左侧栏宽度
                FillRect(dc,&r,sidebar_);
                return 1;
            }
            case WM_CLOSE:
                if (isleave()) {
                    DestroyWindow(window_);
                }
                return 0;
            case WM_DESTROY:
                KillTimer(window_,Tick);
                if (font_) DeleteObject(font_);
                if (heading_) DeleteObject(heading_);
                if (small_) DeleteObject(small_);
                if (background_) DeleteObject(background_);
                if (sidebar_) DeleteObject(sidebar_);
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(window_,msg,wp,lp);
    }
};
LRESULT CALLBACK wndProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        app = static_cast<App*>(cs->lpCreateParams);
        app->attach(window);
        SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->handle(msg,wp,lp) : DefWindowProcW(window,msg,wp,lp);
}
}
int runUI(HINSTANCE instance, int show) {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_LISTVIEW_CLASSES|ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&common);
    static App app(instance); // 窗口关闭前一直保留
    WNDCLASSW wc{};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NumberLogicWindow";
    wc.hCursor = LoadCursorW(nullptr,IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr,IDI_APPLICATION);
    if (!RegisterClassW(&wc)) {
        return 1;
    }
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rect{0,0,W,H};
    AdjustWindowRect(&rect,style,FALSE);
    HWND window = CreateWindowExW(0,wc.lpszClassName,L"数字推理工坊 V1.0.0",style,
        CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,&app);
    if (!window) {
        return 2;
    }
    ShowWindow(window,show);
    UpdateWindow(window);
    MSG msg{};
    while (GetMessageW(&msg,nullptr,0,0)>0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetFocus() == GetDlgItem(window,Guess)) {
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(Submit,BN_CLICKED),0); // 回车提交
            continue;
        }
        if (!IsDialogMessageW(window,&msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}
