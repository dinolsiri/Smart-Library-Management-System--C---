#include <exception>
#include <iostream>

#include "app/Services.hpp"
#include "db/Database.hpp"
#include "db/Migrations.hpp"
#include "ui/TerminalApp.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

int main() {
  try {
    auto config = libraflow::utils::ConfigLoader::load("config.json");
    libraflow::utils::Logger logger("libraflow.log");
    libraflow::db::Database database(config.db_path);
    libraflow::db::Migrator::migrate(database, logger);

    libraflow::app::AppContext context(database, config, logger);
    libraflow::ui::TerminalApp app(context);
    app.run();
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "LibraFlow failed to start: " << ex.what() << '\n';
    return 1;
  }
}
