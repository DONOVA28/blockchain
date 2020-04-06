#include <graphene/chain/cheque_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <fc/uint128.hpp>
#include <boost/range.hpp>

#include <iostream>
#include <iomanip>

namespace graphene { namespace chain {

   void cheque_object::allocate_payees(uint32_t payees_count) {
      payees.resize(payees_count);
   }

   void cheque_object::process_payee(account_id_type payee, database& db)
   {
      // if cheque is already used then exit...
      if (status == cheque_status::cheque_used) { return; }

      int used_count = 0;
      bool check_used = false;
      for (uint32_t i = 0; i < payees.size(); i++)
      {
         // if have unused payee item
         if (!check_used && (payees[i].status == cheque_status::cheque_new))
         {
            // adjust balance for payee user
            db.adjust_balance(payee, asset(amount_payee, asset_id));

            // fill used payee item
            payees[i].status = cheque_status::cheque_used;
            payees[i].payee = payee;
            payees[i].datetime_used = db.head_block_time();

            amount_remaining -= amount_payee;
            check_used = true;
            ++used_count;
         }
         else if (payees[i].status == cheque_status::cheque_used) {
            ++used_count;
         }
      }
      if (used_count == (int)payees.size())
      {
         status = cheque_status::cheque_used;
         datetime_used = db.head_block_time();
      }
   }

} } // graphene::chain

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::cheque_object, (graphene::db::object),
                    (code)
                    (datetime_creation)
                    (datetime_expiration)
                    (datetime_used)
                    (drawer)
                    (amount_payee)
                    (amount_remaining)
                    (asset_id)
                    (status)
                    (payees) )

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::cheque_object::payee_item)
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::cheque_object )
