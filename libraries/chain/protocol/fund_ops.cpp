#include <graphene/chain/protocol/fund_ops.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void fund_options::validate() const
{
   FC_ASSERT( rates_reduction_per_month > -1, "fund.options.rates_reduction_per_month must be greater than -1");
   FC_ASSERT( period > 0, "fund.options.period can't be NULL");

   FC_ASSERT( fund_rates.size(), "fund.options.fund_rates can't be empty!" );
   FC_ASSERT( payment_rates.size(), "fund.options.payment_rates can't be empty!" );
   FC_ASSERT( min_deposit > 0, "fund.options.min_deposit must be greater than 0");

   std::for_each(payment_rates.begin(), payment_rates.end(), [](const fund_options::payment_rate& item) {
      FC_ASSERT( item.period > 0, "each item.period must be greater than 0" );
   });

   std::for_each(fund_rates.begin(), fund_rates.end(), [](const fund_options::fund_rate& item) {
      FC_ASSERT( item.amount > 0, "each item.amount must be greater than 0");
   });

   for (const fund_options::fund_rate& item: fund_rates)
   {
      int count = std::count_if(fund_rates.begin(), fund_rates.end(), [&item](const fund_options::fund_rate& item2){
         return (item.amount == item2.amount);
      });
      FC_ASSERT( (count == 1), "All amounts must be different!" );
   }
}

void fund_create_operation::validate() const
{
   FC_ASSERT( name.length() > 0, "fund.name.length must be > 0");

   options.validate();
};

void fund_update_operation::validate() const {
   options.validate();
}

void fund_refill_operation::validate() const
{
   FC_ASSERT( amount > 0, "amount must be > 0");
}

void fund_deposit_operation::validate() const
{
   FC_ASSERT( amount > 0, "amount must be > 0" );
   FC_ASSERT( period > 0, "period must be > 0" );
}

} } // namespace graphene::chain