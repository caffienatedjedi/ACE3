class CfgWeapons {
    class ACE_ItemCore;
    class InventoryItem_Base_F;

    class ACE_CableTie: ACE_ItemCore {
        displayName = "$STR_ACE_Captives_CableTie";
        descriptionShort = "$STR_ACE_Captives_CableTieDescription";
        model = QUOTE(PATHTOF(models\ace_cabletie.p3d));
        picture = QUOTE(PATHTOF(UI\ace_cabletie_ca.paa));
        scope = 2;
        class ItemInfo: InventoryItem_Base_F {
            mass = 1;
        };
    };
};
