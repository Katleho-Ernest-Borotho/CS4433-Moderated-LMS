// NFR-01 reproducible developer benchmark. Uses a disposable SQLite database only.
// It deliberately measures login, not the one-time fixture provisioning cost.
#include "lms/application/AuthenticationService.h"
#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv)
{
    namespace fs=std::filesystem;
    constexpr int studentCount=800;
    constexpr int trials=20;
    const std::string password="DemoOnly_NotForRealAccounts_2026!";
    const fs::path schema=argc>1?fs::path(argv[1]):fs::path("lms_schema.sql");
    const fs::path temp=fs::temp_directory_path()/
        ("CS4433_NFR01_TEST_ONLY_"+
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".sqlite3");
    int passed=0;
    try {
        {
            lms::SqliteDatabase database(temp.string());
            database.initializeSchemaFromFile(schema.string());
            lms::SQLiteRecordRepository users(database);
            lms::SodiumPasswordHasher passwords;
            // This is an ephemeral performance fixture, never for real deployment:
            // one valid salted hash is copied only to reduce fixture setup cost.
            // The timed account is authenticated through the production hasher.
            const auto fixtureHash=passwords.hash(password);
            for(int index=0;index<studentCount;++index) {
                const auto username="student_"+std::to_string(index+1000);
                users.saveUser(username,fixtureHash,lms::Role::Student);
            }
            lms::AuthenticationService authentication(users,passwords);
            std::cout<<"NFR-01 benchmark: "<<studentCount
                     <<" Student accounts; "<<trials<<" valid logins\n";
            std::cout<<"These times apply ONLY to this machine and test configuration.\n";
            for(int n=0;n<trials;++n) {
                const auto start=std::chrono::steady_clock::now();
                const auto session=authentication.logIn("student_1799",password);
                const auto end=std::chrono::steady_clock::now();
                if(session.role()!=lms::Role::Student)
                    throw std::runtime_error("Unexpected role during benchmark");
                const auto ms=std::chrono::duration<double,std::milli>(end-start).count();
                if(ms<=2000.0)++passed;
                std::cout<<std::setw(2)<<n+1<<": "<<std::fixed
                         <<std::setprecision(2)<<ms<<" ms\n";
            }
        }
        std::error_code ignored;
        fs::remove(temp,ignored);
        std::cout<<"Valid logins <= 2,000 ms: "<<passed<<"/"<<trials<<"\n";
        std::cout<<(passed>=19?"NFR-01 PASS\n":"NFR-01 FAIL (needs investigation)\n");
        return passed>=19?0:2;
    } catch(const std::exception& e) {
        std::error_code ignored;
        fs::remove(temp,ignored);
        std::cerr<<"Benchmark aborted: "<<e.what()<<"\n";
        return 1;
    }
}
