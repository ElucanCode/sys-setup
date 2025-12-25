-- :options
vim.g.have_nerd_font = true
vim.o.number = true
vim.o.splitright = true
vim.o.splitbelow = true
vim.o.relativenumber = true
vim.o.cursorline = true
vim.o.wrap = true
vim.o.scrolloff = 10                                -- Always keep lines above/below cursor
vim.o.sidescrolloff = 10                            -- Always keep lines left/right cursor
-- indentation
local ts = 4
vim.o.tabstop = ts
vim.o.shiftwidth = ts
vim.o.softtabstop = ts
vim.o.expandtab = true
vim.o.smartindent = true
vim.o.autoindent = true
-- search settings
vim.o.ignorecase = true
vim.o.smartcase = true
vim.o.hlsearch = true
vim.o.incsearch = true
-- visuals
vim.o.termguicolors = true                          -- 24-bit colors
vim.o.signcolumn = "auto"
vim.o.colorcolumn = "80,90"                         -- I prefer a line length of 80 as a quasi
                                                    -- max with not a lot longer lines
vim.o.showmatch = true                              -- highlight matching brackets
vim.o.cmdheight = 1
vim.o.completeopt = "menuone,fuzzy"                 -- TODO: Other better settings?
vim.o.showmode = false
vim.o.pumheight = 10                                -- Pop Up Menu height
vim.o.pumblend = 0                                  -- no transparency for pop up menu
vim.o.winblend = 0                                  -- no transparency for floating windows
vim.o.conceallevel = 2
vim.o.concealcursor = ""
vim.o.lazyredraw = true                             -- no redraw during macros
vim.o.winborder = 'rounded'
-- o.synmaxcol = ???
vim.o.spell = true
vim.o.spelllang = "en_us,de_de"
vim.o.backup = false
vim.o.writebackup = false
vim.o.swapfile = false
vim.o.undofile = true
local undodir = vim.env["HOME"] .. "/.cache/nvim/undo_history"
vim.o.undodir = undodir
if vim.fn.isdirectory(undodir) == 0 then            -- create undo dir if it does not exist
    vim.fn.mkdir(vim.o.undodor, "p")
end
vim.o.autoread = true
vim.o.autowrite = false
vim.o.mouse = "a"                                   -- allow resizing of splits
vim.o.foldmethod = "expr"
vim.o.foldexpr = "nvim_treesitter#foldexpr()"
vim.o.foldlevel = 99
vim.g.mapleader = " "
vim.g.maplocalleader = " "

-- :keymaps
local map = vim.keymap.set
map("n", "<leader>n", ":noh<CR>", { desc = "Clear hightlights" })
map("n", "Y", "y$", { desc = "Yank to end of line" })
map("n", "<C-d>", "<C-d>zz", { desc = "Go down and center line" })
map("n", "<C-u>", "<C-u>zz", { desc = "Go up and center line" })
map("n", "n", "nzz", { desc = "Go to next pattern and center line" })
map("n", "N", "Nzz", { desc = "Go to prev pattern and center line" })

map({ "n", "v" }, "<leader>y", [["+y]])
map({ "n", "v" }, "<leader>Y", [["+Y]])

map("n", "<A-K>", ":resize +2<CR>", { desc = "Horizontal resize increase" })
map("n", "<A-J>", ":resize -2<CR>", { desc = "Horizontal resize decrease" })
map("n", "<A-H>", ":vertical resize -2<CR>", { desc = "Vertical resize increase" })
map("n", "<A-L>", ":vertical resize +2<CR>", { desc = "Vertical resize decrease" })

-- Move line up/down
map("n", "<A-j>", ":m .+1<CR>==", { desc = "Move line down" })
map("n", "<A-k>", ":m .-2<CR>==", { desc = "Move line up" })
map("v", "<A-j>", ":m '>+1<CR>gv=gv", { desc = "Move selection down" })
map("v", "<A-k>", ":m '<-2<CR>gv=gv", { desc = "Move selection up" })

-- Better indenting in visual mode
map("v", "<", "<gv", { desc = "Indent left and reselect" })
map("v", ">", ">gv", { desc = "Indent right and reselect" })
map("n", "J", "mzJ`z", { desc = "Join lines and keep cursor position" })

map("t", "<ESC>", "<C-\\><C-n>", { desc = "Exit terminal mode to normal mode" })

vim.opt.listchars = { space = '·', tab = '󰌒 ', eol = '󱞥', }
map("n", "<leader>s", function()
    if vim.opt.list._value then
        vim.opt.list = false
    else
        vim.opt.list = true
    end
end, { desc = "Show space characters" })

local config_init = vim.env["MYVIMRC"]
if config_init then
    map("n", "<leader>c", function()
        vim.cmd("source " .. config_init)
    end, { desc = "Reload config" })
end

-- :autocommand
local cfg_augroup = vim.api.nvim_create_augroup("config", {})
-- Highlight yanked text
vim.api.nvim_create_autocmd("TextYankPost", {
    group = cfg_augroup,
    callback = function() vim.hl.on_yank() end,
})
-- Return to last edit position when reopening a file
vim.api.nvim_create_autocmd("BufReadPost", {
    group = cfg_augroup,
    callback = function()
        local mark = vim.api.nvim_buf_get_mark(0, '"')
        local lcount = vim.api.nvim_buf_line_count(0)
        if mark[1] > 0 and mark[1] <= lcount then
            pcall(vim.api.nvim_win_set_cursor, 0, mark)
        end
    end
})
vim.api.nvim_create_autocmd("TermOpen", {
    group = cfg_augroup,
    callback = function()
        vim.opt_local.number = false
        vim.opt_local.relativenumber = false
        vim.scrolloff = 0
    end,
})
-- automagically resize splits when the window is resized
vim.api.nvim_create_autocmd("VimResized", {
    group = cfg_augroup,
    callback = function()
        vim.cmd("tabdo wincmd =")
    end
})
-- automagically close a terminal when the process exits
vim.api.nvim_create_autocmd("TermClose", {
    group = cfg_augroup,
    callback = function()
        if vim.v.event.status == 0 then
            vim.api.nvim_buf_delete(0, {})
        end
    end
})

vim.api.nvim_create_autocmd("LspAttach", {
    callback = function(args)
        local bufnr = args.buf
        assert(vim.lsp.get_client_by_id(args.data.client_id), "Must have a valid client")

        vim.opt_local.omnifunc = "v:lua.vim.lsp.omnifunc"
        vim.keymap.set("n", "gd", vim.lsp.buf.definition,
            { buffer = bufnr, desc = "Go to definition" })
        vim.keymap.set("n", "gr", vim.lsp.buf.references,
            { buffer = bufnr, desc = "List all references" })
        vim.keymap.set("n", "gD", vim.lsp.buf.declaration,
            { buffer = bufnr, desc = "Go to declaration" })
        vim.keymap.set("n", "gT", vim.lsp.buf.type_definition,
            { buffer = bufnr, desc = "Go the type definition" })
        vim.keymap.set("n", "K", vim.lsp.buf.hover,
            { buffer = bufnr, desc = "Display information" })
        vim.keymap.set("n", "<C-h>", vim.lsp.buf.signature_help,
            { buffer = bufnr, desc = "Signatue help" })
        vim.keymap.set("n", "<leader>/", vim.lsp.buf.workspace_symbol,
            { buffer = bufnr, desc = "Search for symbol" })
        -- use ]d and [d instead
        -- vim.keymap.set("n", "<leader>d", function()
        --         vim.diagnostic.jump({ count = 1, float = true })
        --     end, { buffer = bufnr, desc = "Go to next diagnostic" })
        -- vim.keymap.set("n", "<leader>D", function()
        --         vim.diagnostic.jump({ count = -1, float = true })
        --     end, { buffer = bufnr, desc = "Go to previous diagnostic" })
        vim.keymap.set("n", "<leader>d", vim.diagnostic.setqflist,
            { buffer = bufnr, desc = "List diagnostics" })

        vim.keymap.set("n", "<leader>r", vim.lsp.buf.rename,
            { buffer = bufnr, desc = "Rename symbol" })
        vim.keymap.set("n", "<leader>a", vim.lsp.buf.code_action,
            { buffer = bufnr, desc = "Selection available action" })
    end
})

-- :term :floatingterm
FloatingTerminal = {
    buf = nil,
    win = nil,
    is_open = false,

    rel_width = 0.8,
    rel_height = 0.8,
}

function FloatingTerminal:close()
    self.is_open = false
    if self.win and vim.api.nvim_win_is_valid(self.win) then
        vim.api.nvim_win_close(self.win, false)
    end
end

function FloatingTerminal:open()
    if not self.buf or not vim.api.nvim_buf_is_valid(self.buf) then
        self.buf = vim.api.nvim_create_buf(false, true)
        vim.api.nvim_set_option_value("bufhidden", "hide", { buf = self.buf })
    end

    local w = math.floor(vim.o.columns * self.rel_width)
    local h = math.floor(vim.o.lines * self.rel_height)
    local c = math.floor((vim.o.columns - w) / 2)
    local r = math.floor((vim.o.lines - h) / 2)

    self.win = vim.api.nvim_open_win(self.buf, true, {
        relative = "editor",
        width = w,           height = h,
        row = r,             col = c,
        style = "minimal",   border = "rounded",
    })

    if self.win == 0 then
        vim.cmd('echo "[ERROR]: Failed to create floating window"')
        return
    end

    vim.api.nvim_set_option_value("winblend", 0, { win = self.win })
    vim.api.nvim_set_option_value("winhighlight",
        "Normal:FloatingTermNormal,FloatBorder:FloatingTermBorder",
        { win = self.win })

    vim.api.nvim_set_hl(0, "FloatingTermNormal", { bg = "none" })
    vim.api.nvim_set_hl(0, "FloatingTermBorder", { bg = "none", })

    local has_term = false
    local lines = vim.api.nvim_buf_get_lines(self.buf, 0, -1, false)
    for _, line in ipairs(lines) do
        if line ~= "" then
            has_term = true
            break
        end
    end

    if not has_term then
        local shell = os.getenv("SHELL")
        if not shell then
            vim.notify("Failed to get shell", vim.log.ERROR)
            return
        end
        if not vim.fn.termopen(shell) then
            vim.notify("Failed to create terminal", vim.log.levels.ERROR)
            return
        end
    end

    self.is_open = true
    vim.cmd("startinsert")
    vim.api.nvim_create_autocmd("BufLeave", {
        buffer = self.buf,
        once = true,
        callback = function()
            if self.is_open and vim.api_win_is_valid(self.win) then
                vim.api.nvim_win_close(self.win, false)
                self.is_open = false
            end
        end
    })
end

function FloatingTerminal:toggle()
    if self.is_open then
        self:close()
    else
        self:open()
    end
end

vim.keymap.set("n", "<leader>t", function() FloatingTerminal:toggle() end,
    { noremap = true, silent = true, desc = "Toggle floating terminal" })
vim.keymap.set("t", "<ESC><ESC>",
    function()
        if FloatingTerminal.is_open then
            FloatingTerminal:close()
        end
    end, { noremap = true, silent = true,
            desc = "Close floating terminal from terminal mode" })

Scheme = {
    light = "catppuccin-latte",
    dark = "catppuccin-mocha",
}

---@param scheme string? either light or dark, if nil darkman will be executed
function Scheme:set(scheme)
    if not scheme then
        scheme = vim.fn.trim(vim.fn.system({"darkman", "get"}))
    end
    vim.cmd.colorscheme(self[scheme] or self["light"])
end

-- :plugins :mini
-- Clone 'mini.nvim' manually in a way that it gets managed by 'mini.deps'
local path_package = vim.fn.stdpath('data') .. '/site/'
local mini_path = path_package .. 'pack/deps/start/mini.nvim'
local mini_installed = vim.loop.fs_stat(mini_path) or false

if not mini_installed then
    vim.notify("Installing `mini.nvim`", vim.log.levels.INFO)
    local clone_cmd = {
        'git', 'clone', '--filter=blob:none',
        'https://github.com/echasnovski/mini.nvim', mini_path
    }
    vim.fn.system(clone_cmd)
    if vim.v.shell_error == 0 then
        vim.cmd('packadd mini.nvim | helptags ALL')
        vim.notify("Installing `mini.nvim`", vim.log.levels.INFO)
        mini_installed = true
    end
end

-- skips the plugin configuration if mini is not installed
if not mini_installed then
    -- :TODO: more default keymaps?
    map("n", "<leader>ff", ":FZF<CR> ", { desc = "Find file" })
    map("n", "-", ":Explore<CR>", { desc = "Open file explorer" })
    return
end

require("mini.deps").setup({ path = { package = path_package } })
local add, now, later = MiniDeps.add, MiniDeps.now, MiniDeps.later

-- :plugins :installation
now(function()
    -- colorschemes
    add({ source = "catppuccin/nvim" })
    add({ source = "folke/tokyonight.nvim" })
    add({ source = "nendix/zen.nvim" })
    -- treesitter & lsp
    add({ source = "nvim-treesitter/nvim-treesitter" })
    add({ source = "nvim-treesitter/nvim-treesitter-context" })
    add({ source = "williamboman/mason.nvim" })
    add({ source = "abeldekat/cmp-mini-snippets",
          depends = { "rafamadriz/friendly-snippets" } })
    add({ source = "hrsh7th/nvim-cmp",
          depends = {
                "hrsh7th/cmp-nvim-lsp", "hrsh7th/cmp-path",
                "hrsh7th/cmp-buffer", "onsails/lspkind.nvim",
    }})
    -- utility
    add({ source = "stevearc/oil.nvim" })
    add({ source = "folke/which-key.nvim" })
    add({ source = "hedyhli/outline.nvim" })
    add({ source = "epwalsh/obsidian.nvim",
            depends = { "nvim-lua/plenary.nvim" } })
    add({ source = "tpope/vim-fugitive",
            name = "fugitive" })
    add({ source = "folke/zen-mode.nvim" })
    add({ source = "nvzone/typr",
            depends = { "nvzone/volt" } })
end)

-- :setup :colorscheme
now(function() Scheme:set() end)

-- :setup :oil
now(function()
    local oil = require("oil")
    oil.setup({
        columns = { "mtime", "size", "permissions", "icon", },
        watch_for_changes = true,
        keymaps = {
            ["<leader>h"] = "actions.toggle_hidden",
            ["<leader>s"] = "actions.change_sort",
        },
        view_options = { case_insensitive = true, }
    })

    map("n", "-", "<CMD>Oil<CR>", { desc = "Open directory" })
    map("n", "<leader>-", oil.toggle_float, { desc = "Open directory" })
end)
-- :setup :mini
later(function()
    require("mini.icons").setup()

    require("mini.pick").setup({
        window = {
            config = function()
                local h = math.floor(0.6 * vim.o.lines)
                local w = math.floor(0.6 * vim.o.columns)
                return {
                    anchor = "NW", height = h, width = w,
                    row = math.floor(0.5 * (vim.o.lines - h)),
                    col = math.floor(0.5 * (vim.o.columns - w)),
                    border = "rounded",
                }
            end
        }
    })
    vim.ui.select = function(items, opts, on_choice)
        return MiniPick.ui_select(items, opts, on_choice, {})
    end
    map("n", "<leader>fb", MiniPick.builtin.buffers, { desc = "Find buffer" })
    map("n", "<leader>ff", MiniPick.builtin.files, { desc = "Find file" })
    map("n", "<leader>fg", MiniPick.builtin.grep_live,
        { desc = "Find live in files" })
    map("n", "<leader>ft", function()
        MiniPick.builtin.grep({ pattern = "TODO" })
    end, { desc = "Find Todos" })
    map("n", "<leader>fs", function()
        MiniPick.builtin.grep({ pattern = ".* :\\w+" })
    end, { desc = "Find section" })
    map("n", "<leader>fc", function()
        local colorschemes = vim.fn.getcompletion('', 'color')
        MiniPick.ui_select(colorschemes, {}, function(scheme)
            vim.cmd.colorscheme(scheme)
        end, {})
    end, { desc = "Find and set colorscheme" })

    require("mini.align").setup()

    local hipatterns = require('mini.hipatterns')
    hipatterns.setup({
        highlighters = {
            -- Highlight standalone 'FIXME', 'HACK', 'TODO', 'NOTE'
            fixme = { pattern = '%f[%w]()FIXME()%f[%W]', group = 'MiniHipatternsFixme' },
            hack  = { pattern = '%f[%w]()HACK()%f[%W]',  group = 'MiniHipatternsHack'  },
            todo  = { pattern = '%f[%w]()TODO()%f[%W]',  group = 'MiniHipatternsTodo'  },
            note  = { pattern = '%f[%w]()NOTE()%f[%W]',  group = 'MiniHipatternsNote'  },

            -- Highlight hex color strings (`#rrggbb`) using that color
            hex_color = hipatterns.gen_highlighter.hex_color(),
        },
    })
end)

-- :setup :treesitter
later(function()
    require("nvim-treesitter.configs").setup({
        ensure_installed = {
            "c", "cpp", "lua", "rust", "java", "bash", "python",
            "latex", "typst",
            "markdown", "markdown_inline",
            "json", "toml",
        },
        sync_install = false,
        auto_install = false,
        highlight = { enable = true },
        indent = { enable = true },
        additional_vim_regex_highlighting = false,
    })

    vim.cmd(":TSUpdate")

    require("treesitter-context").setup({})
end)

-- :setup :lsp :completion
later(function()
    local snip = require("mini.snippets")
    snip.setup({ snippets = { snip.gen_loader.from_lang() }})

    require("mason").setup({
        ui = {
            icons = {
                package_installed = "✓",
                package_pending = "…",
                package_uninstalled = "✗"
            }
        },
    })

    local capabilities = require("cmp_nvim_lsp").default_capabilities()

    local servers = {
        "clangd",
        "lua_ls",
        "pylsp",
        "rust_analyzer",
        "tinymist",
        "cmake_language_server",
        "ols",
    }

    for _, name in ipairs(servers) do
        vim.lsp.enable(name)
    end
    vim.lsp.config("*", { capabilities = capabilities })

    local lspkind = require("lspkind")

    local cmp = require("cmp")
    local cmp_select = { behavior = cmp.SelectBehavior.Select }
    cmp.setup({
        mapping = cmp.mapping.preset.insert({
            ["<C-k>"] = cmp.mapping.select_prev_item(cmp_select),
            ["<C-j>"] = cmp.mapping.select_next_item(cmp_select),
            ["<C-l>"] = cmp.mapping.confirm({ select = true }),
            ["<C-o>"] = cmp.mapping.complete(cmp_select),
        }),
        snippet = {
            expand = function(args) -- mini.snippets expands snippets from lsp...
                local insert = MiniSnippets.config.expand.insert or MiniSnippets.default_insert
                insert({ body = args.body }) -- Insert at cursor
                cmp.resubscribe({ "TextChangedI", "TextChangedP" })
                require("cmp.config").set_onetime({ sources = {} })
             end,
        },
        sources = {
            { name = "mini_snippets" },
            { name = "nvim_lsp" },
            { name = "path" },
            { name = "buffer", option = { get_bufnrs = vim.api.nvim_list_bufs } },
        },
        formatting = {
            format = lspkind.cmp_format({
                -- either "text", "symbol", "text_symbol" or "symbol_text"
                mode = "symbol_text",
                maxwidth = { menu = 50, abbr = 50, },
                ellipsis_char = "…",
            })
        },
    })

    vim.diagnostic.config({
        -- update_in_insert = true,
        float = {
            focusable = false,
            style = "minimal",
            border = "rounded",
            -- source = "always",
            header = "",
            prefix = "",
        },
    })
end)

-- :setup :whichkey
later(function()
    require('which-key').setup({
        layout = {
            width = { min = 15, max = 40 },
            spacing = 2,
        },
        icons = { mappings = vim.g.have_nerd_font, },
    })
end)

-- :setup :outline
later(function()
    vim.keymap.set("n", "<leader>o", "<cmd>Outline<CR>", { desc = "Toggle Outline" })
    require('outline').setup({
        outline_window = {
            position = "left",
        },
    })
end)

-- :setup :obsidian
later(function()
    local obsidian = require("obsidian")
    obsidian.setup({
        workspaces = {
            { name = "general", path = "~/Notes" }
        },
        mappings = {
            ["gf"] = {
              action = function()
                return obsidian.util.gf_passthrough()
              end,
              opts = { noremap = false, expr = true, buffer = true },
            },
            ["<leader>on"] = {
                action = function() vim.cmd("ObsidianNew") end,
                opts = { noremap = false, expr = true,
                        buffer = true, desc = "Create new Note" },
            },
        },
    })
end)

-- :setup :zen-mode
later(function()
    map("n", "<leader>z", ":ZenMode<CR>", { desc = "Toggle zen mode" })
end)

-- :setup :typr
later(function()
    require("typr").setup()
end)

-- Old lsp / completion
-- later(function()
--     require('mini.completion').setup({
--         mappings = { scroll_down = '<C-j>', scroll_up = '<C-k>' }
--     })
--     require("mason").setup({
--         ui = {
--             icons = {
--                 package_installed = "✓",
--                 package_pending = "…",
--                 package_uninstalled = "✗"
--             }
--         },
--     })
--     local capabilities = MiniCompletion.get_lsp_capabilities()
--     local servers = {
--         clangd = {},
--         lua_ls = {},
--         pylsp = {},
--         rust_analyzer = {},
--         tinymist = { -- typst lsp
--             offset_encoding = "utf-8",
--             settings = { exportPdf = "never", }
--         },
--     }
--
--     for name, cfg in pairs(servers) do
--         vim.lsp.enable(name)
--     end
--     vim.lsp.config("*", { capabilities = capabilities })
-- end)
