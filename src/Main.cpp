#include <iostream>
#include <stdio.h>
#include <organization_model/OrganizationModel.hpp>
#include <boost/foreach.hpp>
#include <organization_model/ccf/CCF.hpp>

int main(int argc, char** argv)
{
    using namespace owl_om;

    std::string o_filename;

    if(argc == 2)
    {
        o_filename = std::string(argv[1]);
    }

    if(!o_filename.empty())
    {
        OrganizationModel om( o_filename );
        om.setMaximumNumberOfLinks(2);

        using namespace owl_om;
        using namespace owlapi::vocabulary;

        om.createNewInstance(OM::resolve("Sherpa"), true);
        om.createNewInstance(OM::resolve("BaseCamp"), true);
        om.createNewInstance(OM::resolve("BaseCamp"), true);
        om.createNewInstance(OM::resolve("BaseCamp"), true);
        om.createNewInstance(OM::resolve("CREX"), true);
        om.createNewInstance(OM::resolve("CREX"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);
        om.createNewInstance(OM::resolve("PayloadCamera"), true);

        om.refresh(true);
        std::cout << om.getStatistics();
    } else {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    }
}
