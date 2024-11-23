#ifndef PTTT_PATHS_HPP
#define PTTT_PATHS_HPP

#include <filesystem>
#include <string>

namespace paths
{
    static std::filesystem::path get_data_dir() {
        std::filesystem::path current_file_path = __FILE__;
        std::filesystem::path project_dir = current_file_path.parent_path().parent_path().parent_path();
        return project_dir / "data";
    }

    static std::filesystem::path get_checkpoints_dir() {
        std::filesystem::path current_file_path = __FILE__;
        std::filesystem::path project_dir = current_file_path.parent_path().parent_path().parent_path();
        return project_dir / "checkpoints";
    }

    static std::string get_kuhn_descriptor() {
        return get_data_dir() / "kuhn.txt";
    }

    static std::string get_leduc_descriptor() {
        return get_data_dir() / "leduc2.txt";
    }

    static std::string get_rpss_descriptor() {
        return get_data_dir() / "rock_paper_superscissors.txt";
    }
} // namespace pttt

namespace pttt {
    static std::string get_player0_infoset_path() {
        return paths::get_data_dir() / "player0-infoset.txt";
    }

    static std::string get_player1_infoset_path() {
        return paths::get_data_dir() / "player1-infoset.txt";
    }
}

#endif
